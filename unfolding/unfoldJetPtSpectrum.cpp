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
#include "../helpers/math.C"

std::vector<double> getJetPtBinningNonLin(double ptmin = 0., double ptmax = 1000.){
  std::vector<double> result;
  double current = 0.;
  if(current >= ptmin && current <= ptmax) result.push_back(current);
  while(current < 20.){
    current += 1.;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  while(current < 30) {
    current += 2.;
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
  while(current < 200){
    current += 20;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningLin(double ptmin = 0., double ptmax = 1000.){
  std::vector<double> result;
  double current = 0.;
  if(current >= ptmin && current <= ptmax) result.push_back(current);
  while(current < 200.){
    current += 5.;
    if(current < ptmin) continue;
    if(current > ptmax) continue;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinning(double ptmin = 0., double ptmax = 1000.){
  return getJetPtBinningLin(ptmin, ptmax);
}

void SetAllBins(TH2 *&histo, Double_t val, Bool_t over)
{
  Int_t i = 1;
  Int_t maxi = histo->GetNbinsX();
  Int_t j = 1;
  Int_t maxj = histo->GetNbinsY();

  if (over) {
    i--;
    maxi++; 
    j--;
    maxj++; 
  }

  for (; i <= maxi; i++) {
    for (; j <= maxj; j++) {
      histo->SetBinContent(i, j, val);
    }
  }
}

TH2* GetNormalizedResponseMatrix(const TH1* norm, const TH2* oldrm)
{
  // Normalize the y slices (truth) of response matrix using the given 1D histogram

  if (!oldrm)
    return 0;

  TH2* rm = static_cast<TH2*>(oldrm->Clone("rm"));
  SetAllBins(rm, 0, kTRUE);

  Info("GetNormalizedResponseMatrix", "Now normalizing response matrix...");

  for (Int_t y = 1; y <= rm->GetNbinsY(); y++) {
    Double_t integral = oldrm->Integral(1, rm->GetNbinsX(), y, y);
    if (integral == 0)
      continue;
    Double_t scaleFactor = 1;
    if (norm) scaleFactor *= norm->GetBinContent(y);
    scaleFactor /= integral;
    if (scaleFactor == 0)
      continue;
    Info( "GetNormalizedResponseMatrix", "y= %d, integral = %f, scaleFactor = %f", y, integral, scaleFactor);
    for (Int_t x = 1; x <= rm->GetNbinsX(); x++) {
      rm->SetBinContent(x, y, oldrm->GetBinContent(x, y) * scaleFactor);
    }
  }

  return rm;
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

void unfoldJetPtSpectrum(const std::string_view filedata, const std::string_view filemc, int reg){
  double ptmin = 20., ptmax = 60.;
  auto binningdet = getJetPtBinning(ptmin, ptmax), binningpart = getJetPtBinning(ptmin, ptmax);

  // read data
  ROOT::RDataFrame df(GetNameJetSubstructureTree(filedata), filedata);
  auto model = df.Filter(Form("NEFRec < 0.98 &&  PtJetRec > %.1f && PtJetRec < %.1f", ptmin, ptmax)).Histo1D({"hraw", "raw spectrum", static_cast<int>(binningdet.size() -1), binningdet.data()}, "PtJetRec");
  auto hraw = model.GetPtr();

  // read MC
  TH1 *htrue = new TH1D("htrue", "true spectrum", binningpart.size()-1, binningpart.data()),
      *hsmeared = new TH1D("hsmeared", "det mc", binningdet.size()-1, binningdet.data()), 
      *hsmearedClosure = new TH1D("hsmearedClosure", "det mc (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueClosure = new TH1D("htrueClosure", "true spectrum (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueFull = new TH1D("htrueFull", "true spectrum (for closure test)", binningpart.size() - 1, binningpart.data());
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
    for(auto en : mcreader){
      if(*nefrec > 0.98) continue;
      htrueFull->Fill(*ptsim, *weight);
      if(*ptrec > ptmin && *ptrec < ptmax){
        htrue->Fill(*ptsim, *weight);
        hsmeared->Fill(*ptrec, *weight);
        responseMatrix->Fill(*ptrec, *ptsim, *weight);

        double rdm = closuresplit.Uniform();
        if(rdm < 0.2) {
          hsmearedClosure->Fill(*ptrec, *weight);
          htrueClosure->Fill(*ptsim, *weight);
        } else {
          responseMatrixClosure->Fill(*ptrec, *ptsim, *weight);
        }
      }
    }
  }
  auto respnorm = GetNormalizedResponseMatrix(nullptr, responseMatrix), respnormClosure = GetNormalizedResponseMatrix(nullptr, responseMatrixClosure);
  RooUnfoldResponse response(hraw, htrue, respnorm), responseClosure(hsmearedClosure, htrueClosure, respnormClosure);

  std::cout << "Running unfolding" << std::endl;
  RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovariance;
  RooUnfoldSvd unfolder(&response, hraw, reg);
  auto unfolded = unfolder.Hreco(errorTreatment);
  unfolded->SetName("unfolded");
  TH1 *dvec(nullptr);
  if(auto imp = unfolder.Impl()){
    dvec = imp->GetD();
    dvec->SetName("dvector");
  }
  std::cout << "Running MC closure test" << std::endl;
  RooUnfoldSvd unfolderClosure(&responseClosure, hsmearedClosure, reg, 1000, "unfolderClosure", "unfolderClosure");
  auto unfoldedClosure = unfolderClosure.Hreco(errorTreatment);
  unfoldedClosure->SetName("unfoldedClosure");
  TH1 * dvecClosure(nullptr);
  if(auto imp = unfolderClosure.Impl()){
    dvecClosure = imp->GetD();
    dvecClosure->SetName("dvectorClosure");
  }
  std::unique_ptr<TFile> writer(TFile::Open("unfolded1D.root", "RECREATE"));
  htrueFull->Write();
  htrue->Write();
  hsmeared->Write();
  hsmearedClosure->Write();
  responseMatrix->Write();
  responseMatrixClosure->Write();
  hraw->Write();
  unfolded->Write();
  if(dvec) dvec->Write();
  unfoldedClosure->Write();
  if(dvecClosure) dvecClosure->Write();
}