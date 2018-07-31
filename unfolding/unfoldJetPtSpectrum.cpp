#ifndef __CLING__
#include <vector>
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

#include "../helpers/string.C"

std::vector<double> getJetPtBinning(){
  std::vector<double> result;
  double current = 0.;
  result.push_back(current);
  while(current < 20.){
    current += 1.;
    result.push_back(current);
  }
  while(current < 30) {
    current += 2.;
    result.push_back(current);
  } 
  while(current < 40.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 60){
    current += 10;
    result.push_back(current);
  }
  while(current < 200){
    current += 20;
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

void unfoldJetPtSpectrum(const std::string_view filedata, const std::string_view filemc, const std::string_view trigger, double reg){
  auto binningdet = getJetPtBinning();

  // read data
  double ptmin = 20., ptmax = 100.;
  ROOT::RDataFrame df(GetNameJetSubstructureTree(filedata), filedata);
  auto model = df.Filter(Form("NEFRec < 0.98 &&  PtJetRec > %.1f && PtJetRec < %.1f", ptmin, ptmax)).Histo1D({"hraw", "raw spectrum", binningdet.size() -1, binningdet.data()}, "PtJetRec");
  auto hraw = model.GetPtr();

  // read MC
  TH1 *htrue = new TH1D("htrue", "true spectrum", binningdet.size()-1, binningdet.data()),
      *hsmeared = new TH1D("hsmeared", "det mc", binningdet.size()-1, binningdet.data()), 
      *hsmearedClosure = new TH1D("hsmearedClosure", "det mc (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueClosure = new TH1D("htrueClosure", "true spectrum (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueFull = new TH1D("htrueFull", "true spectrum (for closure test)", binningdet.size() - 1, binningdet.data());
  TH2 *responseMatrix = new TH2D("responseMatrix", "response matrix", binningdet.size()-1, binningdet.data(), binningdet.size()-1, binningdet.data()),
      *responseMatrixClosure = new TH2D("responseMatrixClosure", "response matrix (for closure test)", binningdet.size()-1, binningdet.data(), binningdet.size()-1, binningdet.data());

  RooUnfoldResponse response, responseClosure;
  response.Setup(hraw, htrue);
  responseClosure.Setup(hsmearedClosure, htrue);

  {
    TRandom closuresplit;
    std::unique_ptr<TFile> fread(TFile::Open(filemc.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*fread));
    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
                              nefrec(mcreader, "NEFRec"),
                              weight(mcreader, "PythiaWeight");
    for(auto en : mcreader){
      if(*nefrec > 0.98) continue;
      htrueFull->Fill(*ptsim);
      if(*ptrec > ptmin && *ptrec < ptmax){
        htrue->Fill(*ptsim, *weight);
        hsmeared->Fill(*ptrec, *weight);
        responseMatrix->Fill(*ptrec, *ptsim, *weight);
        response.Fill(*ptrec, *ptsim, *weight);

        double rdm = closuresplit.Uniform();
        if(rdm < 0.2) {
          hsmearedClosure->Fill(*ptrec, *weight);
          htrueClosure->Fill(*ptsim, *weight);
        } else {
          responseMatrixClosure->Fill(*ptrec, *ptsim, *weight);
          responseClosure.Fill(*ptrec, *ptsim, *weight);
        }
      }
    }
  }

  RooUnfoldSvd unfolder(&response, hraw, reg);
  auto unfolded = unfolder.Hreco();
  unfolded->SetName("unfolded");
  auto dvec = unfolder.Impl()->GetD();
  dvec->SetName("dvector");

  RooUnfoldSvd unfolderClosure(&responseClosure, hsmearedClosure, reg);
  auto unfoldedClosure = unfolder.Hreco();
  unfoldedClosure->SetName("unfoldedClosure");
  auto dvecClosure = unfolderClosure.Impl()->GetD();
  dvecClosure->SetName("dvectorClosure");

  std::unique_ptr<TFile> writer(TFile::Open("unfolded1D"));
  htrueFull->Write();
  htrue->Write();
  hsmeared->Write();
  hsmearedClosure->Write();
  responseMatrix->Write();
  responseMatrixClosure->Write();
  hraw->Write();
  unfolded->Write();
  dvec->Write();
  unfoldedClosure->Write();
  dvecClosure->Write();
}