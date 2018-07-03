#ifndef __CLING__
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ROOT/TSeq.hxx"
#include "RStringView.h"
#include "TFile.h"
#include "TH2.h"
#include "TKey.h"
#include "TTreeReader.h"

#include "RooUnfoldResponse.h"
#endif

#include "../helpers/string.C"
#include "unfoldingGeneral.cpp"

std::vector<double> MakePtBinningSmeared(std::string_view trigger) {
  std::vector<double> binlimits;
  if(contains(trigger, "INT7")){
    std::cout << "Using binning for trigger INT7\n";
    double ptbins[] = {20, 30, 40, 50, 60, 80, 100, 120};
    for(auto en : ROOT::TSeqI(0, sizeof(ptbins)/sizeof(double))) binlimits.emplace_back(ptbins[en]);
  } else if(contains(trigger, "EJ2")){
    std::cout << "Using binning for trigger EJ2\n";
    double ptbins[] = {60, 70, 80, 100, 120, 140, 160};
    for(auto en : ROOT::TSeqI(0, sizeof(ptbins)/sizeof(double))) binlimits.emplace_back(ptbins[en]);
  } else if(contains(trigger, "EJ1")){
    std::cout << "Using binning for trigger EJ1\n";
    double ptbins[] = {80, 90, 100, 110, 120, 140, 160, 180, 200, 220, 240, 260};
    for(auto en : ROOT::TSeqI(0, sizeof(ptbins)/sizeof(double))) binlimits.emplace_back(ptbins[en]);
  }
  return binlimits;
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

void RunUnfoldingZgV1(const std::string_view filedata, const std::string_view filemc){
  auto ptbinvec_smear = MakePtBinningSmeared(filedata); // Smeared binnning - only in the region one trusts the data
  std::vector<double> ptbinvec_true = {0., 20., 40., 60., 80., 100., 120., 140., 160., 180., 200., 220., 240., 280., 320., 360., 400.}, // True binning, needs overlap to over/underflow bins
                      zgbins = {0., 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5}; // zg must range from 0 to 0.5
  auto dataextractor = [](const std::string_view filedata, double ptsmearmin, double ptsmearmax, TH2 *hraw) {
    std::unique_ptr<TFile> datafilereader(TFile::Open(filedata.data(), "READ"));
    TTreeReader datareader(GetDataTree(*datafilereader));
    TTreeReaderValue<double>  ptrecData(datareader, "PtJetRec"), 
                              zgRecData(datareader, "ZgMeasured");
    for(auto en : datareader){
      if(*ptrecData < ptsmearmin || *ptrecData > ptsmearmax) continue;
      hraw->Fill(*zgRecData, *ptrecData);
    }

  };
  auto mcextractor = [](const std::string_view filename, double ptsmearmin, double ptsmearmax, TH2 *h2true, TH2 *h2smeared, TH2 *h2smearednocuts, TH2 *h2fulleff, RooUnfoldResponse &response, RooUnfoldResponse &responsenotrunc){
    std::unique_ptr<TFile> mcfilereader(TFile::Open(filename.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*mcfilereader));
    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
                              zgRec(mcreader, "ZgMeasured"), 
                              zgSim(mcreader, "ZgTrue"),
                              weight(mcreader, "PythiaWeight");
    for(auto en : mcreader){
      //if(*ptsim > 200.) continue;
      h2fulleff->Fill(*zgSim, *ptsim, *weight);
      h2smearednocuts->Fill(*zgRec, *ptrec, *weight);
      responsenotrunc.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight);

      // apply reconstruction level cuts
      if(*ptrec > ptsmearmax || *ptrec < ptsmearmin) continue;
      h2smeared->Fill(*zgRec, *ptrec, *weight);
      h2true->Fill(*zgSim, *ptsim, *weight);
      response.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight);
    }
  };

  unfoldingGeneral("zg", filedata, filemc, {ptbinvec_true, zgbins, ptbinvec_smear, zgbins}, dataextractor, mcextractor);
}