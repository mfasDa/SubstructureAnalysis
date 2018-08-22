#ifndef __CLING__
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/TTreeProcessorMT.hxx"
#include "ROOT/TSeq.hxx"
#include "RStringView.h"
#include "TFile.h"
#include "TH2.h"
#include "TKey.h"
#include "TRandom.h"
#include "TTreeReader.h"

#include "RooUnfoldResponse.h"
#endif

#include "../helpers/pthard.C"
#include "../helpers/string.C"
#include "../helpers/substructuretree.C"
#include "binnings/binningZg.C"
#include "unfoldingGeneral.cpp"

std::string getTrigger(const std::string_view filedata){
  if(contains(filedata, "INT7")) return "INT7";
  else if(contains(filedata, "EJ2")) return "EJ2";
  else if(contains(filedata, "EJ1")) return "EJ1";
  return "";
}

void RunUnfoldingZgV1_fakebin0_V2(const std::string_view filedata, const std::string_view filemc, double nefcut = 0.98, double fracSmearClosure = 0.5){
  auto trigger = getTrigger(filedata);
  auto ptbinvec_smear = getPtBinningRealistic(trigger), 
       ptbinvec_true = getPtBinningPart(trigger),
       zgbins_smear = getZgBinningFineFake(),
       zgbins_true = getZgBinningFineFake(); //getZgBinningCoarse();
  auto dataextractor = [nefcut](const std::string_view filedata, double ptsmearmin, double ptsmearmax, TH2D *hraw) {
    ROOT::RDataFrame recframe(GetNameJetSubstructureTree(filedata), filedata);
    auto datahist = recframe.Filter(Form("NEFRec < %f && PtJetRec > %f && PtJetRec < %f", nefcut, ptsmearmin, ptsmearmax)).Define("FakeZg", [](double zg) {return zg < 0.1 ? 0.55 : zg; }, {"ZgMeasured"}).Histo2D(*hraw, "FakeZg", "PtJetRec");
    *hraw = *datahist;
  };
  auto mcextractor = [fracSmearClosure, nefcut](const std::string_view filename, double ptsmearmin, double ptsmearmax, TH2 *h2true, TH2 *h2trueClosure, TH2 *h2trueNoClosure, TH2 *h2smeared, TH2 *h2smearedClosure, TH2 *h2smearedNoClosure, TH2 *h2smearednocuts, TH2 *h2fulleff, RooUnfoldResponse &response, RooUnfoldResponse &responsenotrunc, RooUnfoldResponse &responseClosure){
    std::unique_ptr<TFile> mcfilereader(TFile::Open(filename.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*mcfilereader));

    TRandom samplesplitter;
    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
    //                         nefrec(mcreader, "NEFRec"),
                              zgRec(mcreader, "ZgMeasured"), 
                              zgSim(mcreader, "ZgTrue"),
                              weight(mcreader, "PythiaWeight");
    //TTreeReaderValue<int>     pthardbin(mcreader, "PtHardBin");
    for(auto en : mcreader){
      //if(*ptsim > 200.) continue;
      //if(*nefrec >= nefcut) continue;
      //if(IsOutlier(*ptsim, *pthardbin, 10.)) continue;
      double myzgsim = *zgSim < 0.1 ? 0.55 : *zgSim, 
             myzgrec = *zgRec < 0.1 ? 0.55 : *zgRec;
      h2fulleff->Fill(myzgsim, *ptsim, *weight);
      h2smearednocuts->Fill(myzgrec, *ptrec, *weight);
      responsenotrunc.Fill(myzgrec, *ptrec, myzgsim, *ptsim, *weight);

      // apply reconstruction level cuts
      if(*ptrec > ptsmearmax || *ptrec < ptsmearmin) continue;
      h2smeared->Fill(myzgrec, *ptrec, *weight);
      h2true->Fill(myzgsim, *ptsim, *weight);
      response.Fill(myzgrec, *ptrec, myzgsim, *ptsim, *weight);

      // split sample for closure test
      // test sample and response must be statistically independent
      // Split size determined by fraction used for smeared histogram
      auto test = samplesplitter.Uniform();
      if(test < fracSmearClosure) {
        h2smearedClosure->Fill(myzgrec, *ptrec, *weight);
        h2trueClosure->Fill(myzgsim, *ptsim, *weight);
      } else {
        responseClosure.Fill(myzgrec, *ptrec, myzgsim, *ptsim, *weight);
        h2smearedNoClosure->Fill(myzgrec, *ptrec, *weight);
        h2trueNoClosure->Fill(myzgsim, *ptsim, *weight);
      }
    }
  };

  unfoldingGeneral("zg_fakebin0_V2", filedata, filemc, {ptbinvec_true, zgbins_true, ptbinvec_smear, zgbins_smear}, dataextractor, mcextractor);
}