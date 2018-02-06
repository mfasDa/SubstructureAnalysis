#ifndef __CLING__
#include <iostream>
#include <vector>

#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TH2.h>
#include <TTree.h>
#include <TTreeReader.h>
#endif

const std::vector<double> ptbinvec_true = {0., 20., 40., 60., 80., 100., 120., 140., 160., 180., 200., 220., 240., 280., 320., 360., 400.}; // True binning, needs overlap to over/underflow bins

struct SmearPtRange {
  double fMin;
  double fMax;
  TH1D   *fHist;
  
  void InitHist() {
    fHist = new TH1D(Form("sel_%d_%d", int(fMin), int(fMax)), Form("Selected jets for %.1f < p_{t,det} < %.1f", fMin, fMax), ptbinvec_true.size()-1, ptbinvec_true.data()); 
    fHist->SetDirectory(nullptr);
    fHist->Sumw2();
  }
  bool SelectPt(double pt) const {
    return pt >= fMin && pt <= fMax;
  }
  void FillPt(double pt, double weight = 1.) {
    fHist->Fill(pt, weight);
  }
  TH1 *MakeEfficiency(const TH1D *all) {
    auto result = new TH1D(*fHist);
    result->SetDirectory(nullptr);
    result->SetNameTitle(Form("eff_%d_%d", int(fMin), int(fMax)), Form("Selected jets for %.1f < p_{t,det} < %.1f", fMin, fMax));
    result->Divide(fHist, all, 1., 1., "b");
    return result;
  }
};

std::string basename(std::string_view filename) {
  auto mybasename = filename.substr(filename.find_last_of("/")+1);
  return std::string(mybasename);
}

TTree *GetDataTree(TFile &reader) {
  TTree *result(nullptr);
  for(auto k : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
    if(!k) continue;
    if(((TString(k->GetName()).Contains("JetSubstructure")) || (TString(k->GetName()).Contains("jetSubstructure"))) 
       && (k->ReadObj()->IsA() == TTree::Class())) {
      result = dynamic_cast<TTree *>(k->ReadObj());
    }
  }
  std::cout << "Found tree with name " << result->GetName() << std::endl;
  return result;
}

void makeKinematicEfficiency1D(std::string_view filemc){

  TH1D *hall(new TH1D("true", "truef", ptbinvec_true.size()-1, ptbinvec_true.data()));
  hall->SetDirectory(nullptr);
  hall->Sumw2();

  std::vector<SmearPtRange> ranges = {{20., 1000., nullptr}, {20., 100., nullptr}, {60., 1000., nullptr}, {60., 160., nullptr}, {80., 1000., nullptr}, {80., 240., nullptr}};
  for(auto &r : ranges) r.InitHist();

  ////Get the MC////////////////////////
  // TDataFrame not supported in ROOUnfold (yet) - needs TTreeTreader
  // can however be done with Reduce function

  std::unique_ptr<TFile> mcfilereader(TFile::Open(filemc.data(), "READ"));
  TTreeReader mcreader(GetDataTree(*mcfilereader));
  TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                            ptsim(mcreader, "PtJetSim"), 
                            weight(mcreader, "PythiaWeight");
  for(auto en : mcreader){
    hall->Fill(*ptsim, *weight);
    // apply reconstruction level cuts
    for(auto &r : ranges) {
      if(r.SelectPt(*ptrec)) r.FillPt(*ptsim, *weight);
    }
  }

  auto tag  = basename(filemc);
  tag.replace(tag.find(".root"), 5, "");
  tag.replace(tag.find("JetSubstructureTree"), strlen("JetSubstructureTree"), "CutoffStudies");
  std::unique_ptr<TFile> fout(TFile::Open(Form("%s_effkine.root", tag.data()), "RECREATE"));
  fout->cd();
  hall->Write();
  for(auto &r : ranges) r.fHist->Write();
  for(auto &r : ranges) r.MakeEfficiency(hall)->Write();
}
