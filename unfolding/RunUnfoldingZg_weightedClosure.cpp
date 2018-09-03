
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

void RunUnfoldingZg_weightedClosure(const std::string_view filedata, const std::string_view filemc, const std::string_view sysoption, double nefcut = 0.02, double fracSmearClosure = 0.5){
  auto trigger = getTrigger(filedata);
  auto ptbinvec_smear = getPtBinningRealistic(trigger), 
       ptbinvec_true = getPtBinningPart(trigger),
       zgbins_smear = getZgBinningCoarse(), //FineV1(),
       zgbins_true = getZgBinningCoarse(); //FineV1Part(); //getZgBinningCoarse();
  auto dataextractor = [nefcut](const std::string_view filedata, double ptsmearmin, double ptsmearmax, TH2D *hraw, TList *optionals) {
    ROOT::RDataFrame recframe(GetNameJetSubstructureTree(filedata), filedata);
    //auto datahist = recframe.Filter(Form("NEFRec < 1-%f && NEFRec > %f && PtJetRec > %f && PtJetRec < %f", nefcut, nefcut, ptsmearmin, ptsmearmax)).Histo2D(*hraw, "ZgMeasured", "PtJetRec");
    auto datahist = recframe.Filter(Form("PtJetRec > %f && PtJetRec < %f", ptsmearmin, ptsmearmax)).Histo2D(*hraw, "ZgMeasured", "PtJetRec");
    *hraw = *datahist;
  };
  auto mcextractor = [fracSmearClosure, nefcut](const std::string_view filename, double ptsmearmin, double ptsmearmax, TH2 *h2true, TH2 *h2trueClosure, TH2 *h2trueNoClosure, TH2 *h2smeared, TH2 *h2smearedClosure, TH2 *h2smearedNoClosure, TH2 *h2smearednocuts, TH2 *h2fulleff, RooUnfoldResponse &response, RooUnfoldResponse &responsenotrunc, RooUnfoldResponse &responseClosure, TList *optionals){
    std::unique_ptr<TFile> mcfilereader(TFile::Open(filename.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*mcfilereader));

    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
                              nefrec(mcreader, "NEFRec"),
                              zgRec(mcreader, "ZgMeasured"), 
                              zgSim(mcreader, "ZgTrue"),
                              weight(mcreader, "PythiaWeight");
    TTreeReaderValue<int>     pthardbin(mcreader, "PtHardBin");
    TRandom samplesplitter;
    for(auto en : mcreader){
      //if(*nefrec >= 1- nefcut) continue;
      //if(*nefrec < nefcut) continue;
      if(IsOutlier(*ptsim, *pthardbin, 10.)) continue;
      h2fulleff->Fill(*zgSim, *ptsim, *weight);
      h2smearednocuts->Fill(*zgRec, *ptrec, *weight);
      responsenotrunc.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight);

      // apply reconstruction level cuts
      if(*ptrec > ptsmearmax || *ptrec < ptsmearmin) continue;
      h2smeared->Fill(*zgRec, *ptrec, *weight);
      h2true->Fill(*zgSim, *ptsim, *weight);
      response.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight);

      // split sample for closure test
      // test sample and response must be statistically independent
      // Split size determined by fraction used for smeared histogram
      auto test = samplesplitter.Uniform();
      if(test < fracSmearClosure) {
        h2smearedClosure->Fill(*zgRec, *ptrec, *weight);
        h2trueClosure->Fill(*zgSim, *ptsim, *weight);
      } else {
        responseClosure.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight);
        h2smearedNoClosure->Fill(*zgRec, *ptrec, *weight);
        h2trueNoClosure->Fill(*zgSim, *ptsim, *weight);
      }
    }
  };

  auto reweighter = [&sysoption](const TH2 *hdata, TH2 *hsmeared, TH2 *hsmearedclosure) {
      // calculate weight: data / smear per slice pt
      TRandom smearer;
      auto issmeared = contains(sysoption, "smeared");
      for(auto bpt : ROOT::TSeqI(0, hdata->GetYaxis()->GetNbins())){
            std::unique_ptr<TH1> slicedata(hdata->ProjectionX("slicedata", bpt+1, bpt+1)),
                               slicesmear(hsmeared->ProjectionX("slicesmear", bpt+1, bpt+1)),
                               slicesmearclosure(hsmearedclosure->ProjectionX("slicesmearclosure", bpt+1, bpt+1));
            slicedata->Scale(1./slicedata->Integral());
            slicesmear->Scale(1./slicesmear->Integral());
            slicesmearclosure->Scale(1./slicesmearclosure->Integral());
            std::unique_ptr<TH1> weightsmear(histcopy(slicedata.get())), weightsmearclosure(histcopy(slicedata.get()));
            weightsmear->Divide(slicesmear.get());
            weightsmearclosure->Divide(slicesmearclosure.get());
            for(auto bzg : ROOT::TSeqI(0, hsmeared->GetXaxis()->GetNbins())){
                auto weight_standard = issmeared ? smearer.Gaus(weightsmear->GetBinContent(bzg+1), weightsmear->GetBinError(bzg+1)) : weightsmear->GetBinContent(bzg+1);
                auto weight_closure = issmeared ? smearer.Gaus(weightsmearclosure->GetBinContent(bzg+1), weightsmearclosure->GetBinError(bzg+1)) : weightsmearclosure->GetBinContent(bzg+1);
                hsmeared->SetBinContent(bzg+1, bpt+1, hsmeared->GetBinContent(bzg+1, bpt+1) * weight_standard);
                hsmeared->SetBinError(bzg+1, bpt+1, hsmeared->GetBinError(bzg+1, bpt+1) * weight_standard);
                hsmearedclosure->SetBinContent(bzg+1, bpt+1, hsmearedclosure->GetBinContent(bzg+1, bpt+1) * weight_closure);
                hsmearedclosure->SetBinError(bzg+1, bpt+1, hsmearedclosure->GetBinError(bzg+1, bpt+1) * weight_closure);
            }
        }
  };

  unfoldingGeneral("zg", filedata, filemc, {ptbinvec_true, zgbins_true, ptbinvec_smear, zgbins_smear}, dataextractor, mcextractor, reweighter);
}