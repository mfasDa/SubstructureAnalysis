#ifndef __CLING__
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TTreeReader.h>
#include <TRandom.h>

#include "RooUnfoldResponse.h"
#include "RooUnfoldSvd.h"
#include "TSVDUnfold_local.h"
#endif

#include "../helpers/filesystem.C"
#include "../helpers/math.C"
#include "../helpers/root.C"
#include "../helpers/string.C"
#include "../helpers/unfolding.C"

std::vector<double> getJetPtBinningNonLin(double ptmin = 0., double ptmax = 1000.){
  std::vector<double> result;
  double current = 0.;
  if(current >= ptmin && current <= ptmax) result.push_back(current);
  while(current < 20.){
    current += 10.;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  while(current < 40.) {
    current += 5.;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  while(current < 60){
    current += 10;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  while(current < 400){
    current += 20;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningNonLinTrue(double ptmin = 0., double ptmax = 1000.){
  std::vector<double> result;
  double current = 0.;
  if(current >= ptmin && current <= ptmax) result.push_back(current);
  while(current < 20.){
    current += 10.;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  while(current < 40.) {
    current += 10.;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  while(current < 60){
    current += 20;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  while(current < 400){
    current += 40.;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  return result;
}


std::string GetNameJetSubstructureTree(const std::string_view filename){
  std::string result;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())){
    if(!k) continue;
    if((contains(k->GetName(), "JetSubstructure") || contains(k->GetName(), "jetSubstructure")) 
       && (k->ReadObj()->IsA() == TTree::Class())) {
      result = k->GetName(); 
      break;
    }
  }
  std::cout << "Found tree with name " << result << std::endl;
  return result;
}

TTree *GetDataTree(TFile &reader) {
  TTree *result(nullptr);
  for(auto k : TRangeDynCast<TKey>(reader.GetListOfKeys())){
    if(!k) continue;
    if((contains(k->GetName(), "JetSubstructure") || contains(k->GetName(), "jetSubstructure")) 
       && (k->ReadObj()->IsA() == TTree::Class())) {
      result = dynamic_cast<TTree *>(k->ReadObj());
    }
  }
  std::cout << "Found tree with name " << result->GetName() << std::endl;
  return result;
}

std::string getFileTag(const std::string_view inputfile) {
  std::string tag = basename(inputfile);
  std::cout << "Tag: " << tag << std::endl;
  tag.erase(tag.find(".root"), 5);
  tag.erase(tag.find("JetSubstructureTree_"), strlen("JetSubstructureTree_"));
  return tag;
}

void unfoldJetPtSpectrumSvd(const std::string_view filedata, const std::string_view filemc){
  ROOT::EnableImplicitMT(8);
  double ptmin = 20., ptmax = 100.;
  auto binningdet = getJetPtBinningNonLin(ptmin, ptmax), binningpart = getJetPtBinningNonLinTrue();
  std::string outfilename = Form("unfoldedEnergySvd_%s.root", getFileTag(filedata).data());

  // read data
  ROOT::RDataFrame df(GetNameJetSubstructureTree(filedata), filedata);
  auto model = df.Filter(Form("NEFRec < 0.98 &&  PtJetRec > %.1f && PtJetRec < %.1f", ptmin, ptmax)).Histo1D({"hraw", "raw spectrum", static_cast<int>(binningdet.size() -1), binningdet.data()}, "PtJetRec");
  auto hraw = model.GetPtr();

  // read MC
  TH1 *htrue = new TH1D("htrue", "true spectrum", binningpart.size()-1, binningpart.data()),
      *hsmeared = new TH1D("hsmeared", "det mc", binningdet.size()-1, binningdet.data()), 
      *hsmearedClosure = new TH1D("hsmearedClosure", "det mc (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueClosure = new TH1D("htrueClosure", "true spectrum (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueFull = new TH1D("htrueFull", "non-truncated true spectrum", binningpart.size() - 1, binningpart.data()),
      *htrueFullClosure = new TH1D("htrueFullClosure", "non-truncated true spectrum (for closure test)", binningpart.size() - 1, binningpart.data());
  TH2 *responseMatrix = new TH2D("responseMatrix", "response matrix", binningdet.size()-1, binningdet.data(), binningpart.size()-1, binningpart.data()),
      *responseMatrixClosure = new TH2D("responseMatrixClosure", "response matrix (for closure test)", binningdet.size()-1, binningdet.data(), binningpart.size()-1, binningpart.data());

  {
    TRandom closuresplit;
    std::unique_ptr<TFile> fread(TFile::Open(filemc.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*fread));
    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
                              nefrec(mcreader, "NEFRec"),
                              weight(mcreader, "PythiaWeight");
    bool closureUseSpec;
    for(auto en : mcreader){
      if(*nefrec > 0.98) continue;
      double rdm = closuresplit.Uniform();
      closureUseSpec = (rdm < 0.2);
      htrueFull->Fill(*ptsim, *weight);
      if(closureUseSpec) htrueFullClosure->Fill(*ptsim, *weight);
      if(*ptrec > ptmin && *ptrec < ptmax){
        htrue->Fill(*ptsim, *weight);
        hsmeared->Fill(*ptrec, *weight);
        responseMatrix->Fill(*ptrec, *ptsim, *weight);
        if(closureUseSpec) {
          hsmearedClosure->Fill(*ptrec, *weight);
          htrueClosure->Fill(*ptsim, *weight);
        } else {
          responseMatrixClosure->Fill(*ptrec, *ptsim, *weight);
        }
      }
    }
  }

  // Calculate kinematic efficiency
  auto effKine = histcopy(htrue);
  effKine->SetDirectory(nullptr);
  effKine->SetName("effKine");
  effKine->Divide(effKine, htrueFull, 1., 1., "b");

  // Normalize the prior spectrum
  auto testbin = htrueFull->GetXaxis()->FindBin(41.);
  double oneovercFull = htrueFull->GetBinContent(testbin) / TMath::Power(htrueFull->GetBinError(testbin), 2),
         oneovercFullClosure = htrueFullClosure->GetBinContent(testbin) / TMath::Power(htrueFullClosure->GetBinError(testbin), 2);
  auto priorsFull = histcopy(htrueFull), priorsClosure = histcopy(htrueFullClosure);
  priorsFull->Scale(oneovercFull);
  priorsFull->SetName("priorsFull");
  priorsClosure->Scale(oneovercFullClosure);
  priorsClosure->SetName("priorsClosure");

  // Normalize response matrices
  Normalize2D(responseMatrix); Normalize2D(responseMatrixClosure);  

  // Build response
  RooUnfoldResponse response(nullptr, priorsFull, responseMatrix, "response"), responseClosure(nullptr, priorsClosure, responseMatrixClosure, "responseClosure");

  std::unique_ptr<TFile> writer(TFile::Open(outfilename.data(), "RECREATE"));
  htrueFull->Write();
  htrueFullClosure->Write();
  htrue->Write();
  hsmeared->Write();
  hsmearedClosure->Write();
  responseMatrix->Write();
  responseMatrixClosure->Write();
  hraw->Write();
  effKine->Write();
  priorsFull->Write();
  priorsClosure->Write();

  RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovToy;//ariance;
  for(auto reg : ROOT::TSeqI(0, hraw->GetXaxis()->GetNbins())){
    std::cout << "Regularization " << reg << "\n================================================================\n";
    std::cout << "Running unfolding" << std::endl;
    RooUnfoldSvd unfolder(&response, hraw, reg);
    auto unfolded = unfolder.Hreco(errorTreatment);
    unfolded->SetName(Form("unfoldedReg%d", reg));
    TH1 *dvec(nullptr);
    if(auto imp = unfolder.Impl()){
      dvec = imp->GetD();
      dvec->SetName(Form("dvectorReg%d", reg));
    }
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Running MC closure test" << std::endl;
    RooUnfoldSvd unfolderClosure(&responseClosure, hsmearedClosure, reg, 1000, "unfolderClosure", "unfolderClosure");
    auto unfoldedClosure = unfolderClosure.Hreco(errorTreatment);
    unfoldedClosure->SetName(Form("unfoldedClosureReg%d", reg));
    TH1 * dvecClosure(nullptr);
    if(auto imp = unfolderClosure.Impl()){
      dvecClosure = imp->GetD();
      dvecClosure->SetName(Form("dvectorClosureReg%d", reg));
    }
    writer->mkdir(Form("regularization%d", reg));
    writer->cd(Form("regularization%d", reg));
    unfolded->Write();
    if(dvec) dvec->Write();
    unfoldedClosure->Write();
    if(dvecClosure) dvecClosure->Write();
  }
}