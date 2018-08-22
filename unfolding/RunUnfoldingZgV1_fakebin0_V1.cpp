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

const double fakeweight = 30.;
const bool doFakeResponseMatrix = false;

void RunUnfoldingZgV1_fakebin0_V1(const std::string_view filedata, const std::string_view filemc, double nefcut = 0.98, double fracSmearClosure = 0.5){
  auto trigger = getTrigger(filedata);
  auto ptbinvec_smear = getPtBinningRealistic(trigger), 
       ptbinvec_true = getPtBinningPart(trigger),
       zgbins_smear = getZgBinningFine(),
       zgbins_true = getZgBinningFine(); //getZgBinningCoarse();
  auto dataextractor = [nefcut](const std::string_view filedata, double ptsmearmin, double ptsmearmax, TH2D *hraw) {
    ROOT::RDataFrame recframe(GetNameJetSubstructureTree(filedata), filedata);
    auto datahist = recframe.Filter(Form("NEFRec < %f && PtJetRec > %f && PtJetRec < %f", nefcut, ptsmearmin, ptsmearmax)).Histo2D(*hraw, "ZgMeasured", "PtJetRec");
    *hraw = *datahist;
    // Scale first bin
    for(auto o : ROOT::TSeqI(0, hraw->GetYaxis()->GetNbins())) {
      hraw->SetBinContent(1, o+1, hraw->GetBinContent(1, o+1) * fakeweight);
      hraw->SetBinError(1, o+1, hraw->GetBinError(1, o+1) * fakeweight); 
    }
  };
  auto mcextractor = [fracSmearClosure, nefcut](const std::string_view filename, double ptsmearmin, double ptsmearmax, TH2 *h2true, TH2 *h2trueClosure, TH2 *h2trueNoClosure, TH2 *h2smeared, TH2 *h2smearedClosure, TH2 *h2smearedNoClosure, TH2 *h2smearednocuts, TH2 *h2fulleff, RooUnfoldResponse &response, RooUnfoldResponse &responsenotrunc, RooUnfoldResponse &responseClosure){
    std::unique_ptr<TFile> mcfilereader(TFile::Open(filename.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*mcfilereader));

    TRandom samplesplitter;
    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
    //                          nefrec(mcreader, "NEFRec"),
                              zgRec(mcreader, "ZgMeasured"), 
                              zgSim(mcreader, "ZgTrue"),
                              weight(mcreader, "PythiaWeight");
    //TTreeReaderValue<int>     pthardbin(mcreader, "PtHardBin");
    for(auto en : mcreader){
      //if(*ptsim > 200.) continue;
      //if(*nefrec >= nefcut) continue;
      //if(IsOutlier(*ptsim, *pthardbin, 10.)) continue;
      auto trueweight = ((*zgSim < 0.1) && doFakeResponseMatrix) ? fakeweight * (*weight) : *weight;
      h2fulleff->Fill(*zgSim, *ptsim, trueweight);
      h2smearednocuts->Fill(*zgRec, *ptrec, *weight);
      responsenotrunc.Fill(*zgRec, *ptrec, *zgSim, *ptsim, trueweight);

      // apply reconstruction level cuts
      if(*ptrec > ptsmearmax || *ptrec < ptsmearmin) continue;
      h2smeared->Fill(*zgRec, *ptrec, *weight);
      h2true->Fill(*zgSim, *ptsim, trueweight);
      response.Fill(*zgRec, *ptrec, *zgSim, *ptsim, trueweight);

      // split sample for closure test
      // test sample and response must be statistically independent
      // Split size determined by fraction used for smeared histogram
      auto test = samplesplitter.Uniform();
      if(test < fracSmearClosure) {
        h2smearedClosure->Fill(*zgRec, *ptrec, *weight);
        h2trueClosure->Fill(*zgSim, *ptsim, trueweight);
      } else {
        responseClosure.Fill(*zgRec, *ptrec, *zgSim, *ptsim, trueweight);
        h2smearedNoClosure->Fill(*zgRec, *ptrec, *weight);
        h2trueNoClosure->Fill(*zgSim, *ptsim, trueweight);
      }
    }

    // Upscale also smeared distributions by scale
    for(auto o : ROOT::TSeqI(0, h2smeared->GetYaxis()->GetNbins())) {
      h2smeared->SetBinContent(1, o+1, h2smeared->GetBinContent(1, o+1) * fakeweight);
      h2smeared->SetBinError(1, o+1, h2smeared->GetBinError(1, o+1) * fakeweight); 
    } 

    for(auto o : ROOT::TSeqI(0, h2smearedClosure->GetYaxis()->GetNbins())) {
      h2smearedClosure->SetBinContent(1, o+1, h2smearedClosure->GetBinContent(1, o+1) * fakeweight);
      h2smearedClosure->SetBinError(1, o+1, h2smearedClosure->GetBinError(1, o+1) * fakeweight); 
    } 

    for(auto o : ROOT::TSeqI(0, h2smearedNoClosure->GetYaxis()->GetNbins())) {
      h2smearedNoClosure->SetBinContent(1, o+1, h2smearedNoClosure->GetBinContent(1, o+1) * fakeweight);
      h2smearedNoClosure->SetBinError(1, o+1, h2smearedNoClosure->GetBinError(1, o+1) * fakeweight); 
    } 
  };

  unfoldingGeneral("zg_fakebin0_V1", filedata, filemc, {ptbinvec_true, zgbins_true, ptbinvec_smear, zgbins_smear}, dataextractor, mcextractor);
}