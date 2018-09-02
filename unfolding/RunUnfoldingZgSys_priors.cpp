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

void RunUnfoldingZgSys_priors(const std::string_view filedata, const std::string_view filemc, const std::string_view systematic, double nefcut = 0.02, double fracSmearClosure = 0.5){
  auto trigger = getTrigger(filedata);
  auto ptbinvec_smear = getPtBinningRealistic(trigger), 
       ptbinvec_true = getPtBinningPart(trigger),
       zgbins_smear = getZgBinningCoarse(),
       zgbins_true = getZgBinningCoarse();
  auto dataextractor = [nefcut](const std::string_view filedata, double ptsmearmin, double ptsmearmax, TH2D *hraw, TList *optionals) {
    ROOT::RDataFrame recframe(GetNameJetSubstructureTree(filedata), filedata);
    //auto datahist = recframe.Filter(Form("NEFRec < 1-%f && NEFRec > %f && PtJetRec > %f && PtJetRec < %f", nefcut, nefcut, ptsmearmin, ptsmearmax)).Histo2D(*hraw, "ZgMeasured", "PtJetRec");
    auto datahist = recframe.Filter(Form("PtJetRec > %f && PtJetRec < %f", ptsmearmin, ptsmearmax)).Histo2D(*hraw, "ZgMeasured", "PtJetRec");
    *hraw = *datahist;
  };
  auto mcextractor = [fracSmearClosure, nefcut](const std::string_view filename, double ptsmearmin, double ptsmearmax, TH2 *h2true, TH2 *h2trueClosure, TH2 *h2trueNoClosure, TH2 *h2smeared, TH2 *h2smearedClosure, TH2 *h2smearedNoClosure, TH2 *h2smearednocuts, TH2 *h2fulleff, RooUnfoldResponse &response, RooUnfoldResponse &responsenotrunc, RooUnfoldResponse &responseClosure, TList *optionals){
    TH2 *weighthist(nullptr);
    {
      std::string repo = "/data1/markus/Fulljets/pp_13TeV/Substructuretree/data_mc/20180620_corr2017/unfolded_zg/NEFcut/no_cut";
      std::string weightfilename = basename(filename);
      weightfilename.erase(weightfilename.find("merged.root"), 11);
      weightfilename += "unfolded_zg.root";
      std::string weightfile = repo + "/" + weightfilename;
      std::unique_ptr<TFile> weightreader(TFile::Open(weightfile.data(), "READ"));
      TH2 *truehistPrior = static_cast<TH2 *>(weightreader->Get("true"));
      truehistPrior->SetDirectory(nullptr);
      weightreader->cd("iteration4");
      TH2 *unfoldedhistPrior = static_cast<TH2 *>(gDirectory->Get("zg_unfolded_iter4"));
      unfoldedhistPrior->SetDirectory(nullptr);

      std::unique_ptr<TH1> integralsTrue(truehistPrior->ProjectionY("integralsTrue")),
                           integralsUnfolded(unfoldedhistPrior->ProjectionY("integralsUnfolded"));
      // renormalize by integral
      for(auto b : ROOT::TSeqI(0, truehistPrior->GetYaxis()->GetNbins())){
        auto scaletrue = integralsTrue->GetBinContent(b+1),
             scaleunfolded = integralsUnfolded->GetBinContent(b+1);
        for(auto c : ROOT::TSeqI(0, truehistPrior->GetXaxis()->GetNbins())) {
          truehistPrior->SetBinContent(c+1, b+1, truehistPrior->GetBinContent(c+1, b+1) / scaletrue);
          truehistPrior->SetBinError(c+1, b+1, truehistPrior->GetBinContent(c+1, b+1) / scaletrue);
          unfoldedhistPrior->SetBinContent(c+1, b+1, unfoldedhistPrior->GetBinContent(c+1, b+1) / scaleunfolded);
          unfoldedhistPrior->SetBinError(c+1, b+1, unfoldedhistPrior->GetBinContent(c+1, b+1) / scaleunfolded);
        }
      }
      weighthist = static_cast<TH2 *>(histcopy(unfoldedhistPrior));
      weighthist->SetName("priorweights");
      weighthist->Divide(truehistPrior);
    }
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
      if(*ptsim > 200.) continue;
      //if(*nefrec >= 1- nefcut) continue;
      //if(*nefrec < nefcut) continue;
      if(IsOutlier(*ptsim, *pthardbin, 10.)) continue;
      auto priorweight = weighthist->GetBinContent(weighthist->GetXaxis()->FindBin(*zgSim), weighthist->GetYaxis()->FindBin(*ptsim));
      h2fulleff->Fill(*zgSim, *ptsim, *weight * priorweight);
      h2smearednocuts->Fill(*zgRec, *ptrec, *weight);
      responsenotrunc.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight * priorweight);

      // apply reconstruction level cuts
      if(*ptrec > ptsmearmax || *ptrec < ptsmearmin) continue;
      h2smeared->Fill(*zgRec, *ptrec, *weight);
      h2true->Fill(*zgSim, *ptsim, *weight * priorweight);
      response.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight * priorweight);

      // split sample for closure test
      // test sample and response must be statistically independent
      // Split size determined by fraction used for smeared histogram
      auto test = samplesplitter.Uniform();
      if(test < fracSmearClosure) {
        h2smearedClosure->Fill(*zgRec, *ptrec, *weight);
        h2trueClosure->Fill(*zgSim, *ptsim, *weight * priorweight);
      } else {
        responseClosure.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight * priorweight);
        h2smearedNoClosure->Fill(*zgRec, *ptrec, *weight);
        h2trueNoClosure->Fill(*zgSim, *ptsim, *weight * priorweight);
      }
    }
    optionals->Add(weighthist);
  };

  unfoldingGeneral("zg", filedata, filemc, {ptbinvec_true, zgbins_true, ptbinvec_smear, zgbins_smear}, dataextractor, mcextractor);
}