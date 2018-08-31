//=====================================================================-*-C++-*-
// File and Version Information:
//      $Id: RooUnfoldExample.cxx 279 2011-02-11 18:23:44Z T.J.Adye $
//
// Description:
//      Simple example usage of the RooUnfold package using toy MC.
//
// Authors: Tim Adye <T.J.Adye@rl.ac.uk> and Fergus Wilson <fwilson@slac.stanford.edu>
//
//==============================================================================

#if !defined(__CLING__) 
#include <algorithm>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "ROOT/TSeq.hxx"
#include "ROOT/TProcessExecutor.hxx"
#include "RStringView.h"
#include "TFile.h"
#include "TH2D.h"
#include "TROOT.h"
#include "TStopwatch.h"

#include "RooUnfoldResponse.h"
#include "RooUnfoldBayes.h"
//#include "RooUnfoldTestHarness2D.h"
#endif

#include "../helpers/filesystem.C"
#include "../helpers/unfolding.C"

struct binning {
  std::vector<double> binpttrue;
  std::vector<double> binshapetrue;
  std::vector<double> binptsmear;
  std::vector<double> binshapesmear;
};

using datafunction = std::function<void (const std::string_view fiilename, double ptsmearmin, double ptsmearmax, TH2D *hraw)>;
using mcfunction = std::function<void (const std::string_view filename, double ptsmearmin, double ptsmearmax, TH2 *h2true, TH2 *h2trueClosure, TH2 *h2trueNoClosure, TH2 *h2smeared, TH2 *h2smearedClosure, TH2 *h2smearedNoClosure, TH2 *h2smearednocuts, TH2 *h2fulleff, RooUnfoldResponse &resp, RooUnfoldResponse &responsetrunc, RooUnfoldResponse &responseClosure)>;
using reweightfunction = std::function<void (const TH2 *datafunction, TH2 *smeared, TH2 *smearedclosure)>;

void unfoldingGeneral(const std::string_view observable, const std::string_view filedata, std::string_view filemc, const binning &histbinnings, datafunction dataextractor, mcfunction mcextractor, reweightfunction reweighter = nullptr, Bool_t enableImplicitMT = false){
  ROOT::EnableThreadSafety();
  if(enableImplicitMT){
    std::cout << "Using implicit MT" << std::endl;
    ROOT::EnableImplicitMT(10);
  } else {
    std::cout << "Using explicit MT" << std::endl;
  }
  ///////////////////parameter setting
  RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovariance;

  const auto &binpttrue = histbinnings.binpttrue, &binptsmear = histbinnings.binptsmear, &binshapetrue = histbinnings.binshapetrue, &binshapesmear = histbinnings.binshapesmear;
  TH2D *hraw(new TH2D("hraw", "hraw", binshapesmear.size()-1, binshapesmear.data(), binptsmear.size()-1, binptsmear.data())),
       *h2smeared(new TH2D("smeared", "smeared", binshapesmear.size()-1, binshapesmear.data(), binptsmear.size()-1, binptsmear.data())), //detector measured level but no cuts
       *h2smearedClosure(new TH2D("smearedClosure", "smeared, for MC closure test", binshapesmear.size()-1, binshapesmear.data(), binptsmear.size()-1, binptsmear.data())), //detector measured level but no cuts
       *h2smearedNoClosure(new TH2D("smearedNoClosure", "smeared, jets used in response matrix", binshapesmear.size()-1, binshapesmear.data(), binptsmear.size()-1, binptsmear.data())), //detector measured level but no cuts
       *h2smearednocuts(new TH2D("smearednocuts", "smearednocuts", binshapetrue.size()-1, binshapetrue.data(), binpttrue.size()-1, binpttrue.data())),  //true correlations with measured cuts
       *h2true(new TH2D("true", "true", binshapetrue.size()-1, binshapetrue.data(), binpttrue.size()-1, binpttrue.data())),   //full true correlation
       *h2trueClosure(new TH2D("trueClosure", "true, for MC closure test", binshapetrue.size()-1, binshapetrue.data(), binpttrue.size()-1, binpttrue.data())),   //full true correlation
       *h2trueNoClosure(new TH2D("trueNoClosure", "true, jets used in response matrix", binshapetrue.size()-1, binshapetrue.data(), binpttrue.size()-1, binpttrue.data())),   //full true correlation
       *h2fulleff(new TH2D("truefull", "truefull", binshapetrue.size()-1, binshapetrue.data(), binpttrue.size()-1, binpttrue.data()));

  hraw->Sumw2();
  h2smeared->Sumw2();
  h2true->Sumw2();
  h2trueClosure->Sumw2();
  h2fulleff->Sumw2();
  h2smearednocuts->Sumw2();
  h2smearedClosure->Sumw2();

  RooUnfoldResponse response, responsenotrunc, responseMCclosure;
  response.Setup(h2smeared, h2true);
  responsenotrunc.Setup(h2smearednocuts, h2fulleff);
  responseMCclosure.Setup(h2smeared, h2true);

  // define reconstruction level cuts
  auto smearptmin = *(std::min_element(binptsmear.begin(), binptsmear.end()));
  auto smearptmax = *(std::max_element(binptsmear.begin(), binptsmear.end()));

  if(enableImplicitMT) {
    // Read data an MC single-threaded as they are paralellized implicitly

    //////////GET THE DATA////////////
    std::cout << "Data reader: Fill histograms from data" << std::endl;
    TStopwatch timerData;
    timerData.Start();
    dataextractor(filedata, smearptmin, smearptmax, hraw);
    timerData.Stop();
    std::cout << "Data reader: Data ready, duration " << timerData.RealTime() << std::endl;

    ////Get the MC////////////////////////
    std::cout << "MCthread: Fill histograms from simulation" << std::endl;
    TStopwatch timerMC;
    timerMC.Start();
    mcextractor(filemc, smearptmin, smearptmax, h2true, h2trueClosure, h2trueNoClosure, h2smeared, h2smearedClosure, h2smearedNoClosure, h2smearednocuts, h2fulleff, response, responsenotrunc, responseMCclosure);
    timerMC.Stop();
    std::cout << "MCthread: Response ready, duration " << timerMC.RealTime() << std::endl;
  } else {
    /****
     * Explicit multithreading
     * Reading data and MC in parallel
     * Usign one thread for data processing and one thread for MC processing
     */

    //////////GET THE DATA////////////
    std::thread datathread([&]() {
      std::cout << "Datathread: Fill histograms from data" << std::endl;
      TStopwatch timer;
      timer.Start();
      dataextractor(filedata, smearptmin, smearptmax, hraw);
      timer.Stop();
      std::cout << "Datathread: Data ready, duration " << timer.RealTime() << std::endl;
    });

    ////Get the MC////////////////////////
    std::thread mcthread([&](){
      std::cout << "MCthread: Fill histograms from simulation" << std::endl;
      TStopwatch timer;
      timer.Start();
      mcextractor(filemc, smearptmin, smearptmax, h2true, h2trueClosure, h2trueNoClosure, h2smeared, h2smearedClosure, h2smearedNoClosure, h2smearednocuts, h2fulleff, response, responsenotrunc, responseMCclosure);
      timer.Stop();
      std::cout << "MCthread: Response ready, duration " << timer.RealTime() << std::endl;
    });
    datathread.join();
    mcthread.join();
  }

  // reweight
  if(reweighter){
    reweighter(hraw, h2smeared, h2smearedClosure);
  }

  /////////COMPUTE KINEMATIC EFFICIENCIES////////////////////
  std::vector<TH1 *> efficiencies;
  for(auto b : ROOT::TSeqI(1, h2true->GetYaxis()->GetNbins()+1)){
    // ignore bins which are outside the kinematic range of the measurement
    // if(h2true->GetYaxis()->GetBinUpEdge(b) < smearptmin || h2true->GetYaxis()->GetBinLowEdge(b) > smearptmax) continue;
    std::unique_ptr<TH1> truncated(h2true->ProjectionX("truncated", b, b)), 
                         full(h2fulleff->ProjectionX("full", b, b));
    TH1 *efficiency = static_cast<TH1 *>(truncated->Clone(Form("efficiency_%d_%d", int(h2true->GetYaxis()->GetBinLowEdge(b)), int(h2true->GetYaxis()->GetBinUpEdge(b)))));
    efficiency->Sumw2();
    efficiency->SetDirectory(nullptr);
    efficiency->Divide(full.get());
    efficiencies.emplace_back(efficiency);
  }

  using resultformat = std::tuple<int, TH2 *, TH2 *, TH2 *, TH2 *, TH2 *, TH2 *, std::vector<TH2 *>, std::vector<TH2 *>>;
  const Int_t NWORKERS = 10;
  const Int_t MAXITERATIONS = 35;
  auto workitem = [&](int workerID) {
    std::vector<resultformat> result;
    int nstep = 0;
    while(true) {
      auto niter = nstep * NWORKERS + workerID + 1;
      if(niter > MAXITERATIONS) break;

      std::cout << "iteration" << niter << std::endl;
      std::cout << "==============Unfold h1=====================" << std::endl;

      RooUnfoldBayes unfold(&response, hraw, niter); // OR
      auto hunf = (TH2D *)unfold.Hreco(errorTreatment);
      hunf->SetName(Form("%s_unfolded_iter%d", observable.data(), niter));

      // MC closure test
      RooUnfoldBayes unfoldClosure(&responseMCclosure, h2smearedClosure, niter);
      auto hunfClosure = (TH2 *)unfoldClosure.Hreco(errorTreatment);
      hunfClosure->SetName(Form("%s_unfoldedClosure_iter%d", observable.data(), niter));

      // MC closure test (self closure  - use full smeared and full response)
      // not statistically independent any more
      RooUnfoldBayes unfoldSelfClosure(&response, h2smeared, niter);
      auto hunfSelfClosure = (TH2 *)unfoldSelfClosure.Hreco(errorTreatment);
      hunfSelfClosure->SetName(Form("%s_unfoldedSelfClosure_iter%d", observable.data(), niter));

      // FOLD BACK
      auto hfold = Refold(hraw, hunf, response);
      hfold->SetName(Form("%s_folded_iter%d", observable.data(), niter));

      auto hfoldClosure = Refold(h2smearedClosure, hunfClosure, responseMCclosure);
      hfoldClosure->SetName(Form("%s_foldedClosure_iter%d", observable.data(), niter));

      auto hfoldSelfClosure = Refold(h2smeared, hunfSelfClosure, response);
      hfoldSelfClosure->SetName(Form("%s_foldedSelfClosure_iter%d", observable.data(), niter));

      //CheckNormalized(response, sizeof(zgbins)/sizeof(double)-1, sizeof(zgbins)/sizeof(double)-1, ptbinvec_true.size()-1, ptbinvec_smear.size()-1);

      TMatrixD covmat = unfold.Ereco((RooUnfold::ErrorTreatment)RooUnfold::kCovariance);
      std::vector<TH2 *> shapematrices, ptmatrices, responseMatricesShape;
      for (auto k : ROOT::TSeqI(0, h2true->GetNbinsX()))
        ptmatrices.emplace_back(CorrelationHistPt(covmat, Form("pearsonmatrix_iter%d_bin%s%d", niter, observable.data(), k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k));

      for (auto k : ROOT::TSeqI(0, h2true->GetNbinsY()))
        shapematrices.emplace_back(CorrelationHistShape(covmat, Form("pearsonmatrix_iter%d_binpt%d", niter, k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k));
      
      result.emplace_back(std::make_tuple(niter, hunf, hfold, hunfClosure, hunfSelfClosure, hfoldClosure, hfoldSelfClosure, shapematrices, ptmatrices));
      nstep++;
    }
    return result;
  };
  auto reducer = [](const std::vector<std::vector<resultformat>> &data){
    std::vector<resultformat> result;
    for(auto d : data) {
      for(auto e : d) {
        result.emplace_back(e);
      }
    }
    std::sort(result.begin(), result.end(), [](const resultformat &first, const resultformat &second) { return std::get<0>(first) < std::get<0>(second); } );
    return result;
  };

  ROOT::TProcessExecutor pool(std::min(NWORKERS, MAXITERATIONS));
  auto unfoldingresult = pool.MapReduce(workitem, ROOT::TSeqI(0, std::min(NWORKERS, MAXITERATIONS)), reducer);

  auto tag = basename(filedata);
  tag.replace(tag.find(".root"), 5, "");
  std::unique_ptr<TFile> fout(TFile::Open(Form("%s_unfolded_%s.root", tag.data(), observable.data()), "RECREATE"));
  fout->cd();
  
  for(auto e : efficiencies) e->Write();

  hraw->Write();
  h2smeared->Write();
  h2smearedClosure->Write();
  h2smearedNoClosure->Write();
  h2true->Write();
  h2trueClosure->Write();
  h2trueNoClosure->Write();
  h2fulleff->Write();

  auto responseMatrix2D = response.Hresponse();
  responseMatrix2D->SetName("ResponseMatrix2D");
  responseMatrix2D->Write();

  // project response matrices
  fout->mkdir("sliceresponse");
  fout->cd("sliceresponse");
  for(auto binpttrue : ROOT::TSeqI(0, h2true->GetYaxis()->GetNbins())){
    for(auto binptsmear : ROOT::TSeqI(0, h2smeared->GetYaxis()->GetNbins())){
      sliceRepsonseObservable(response, observable.data(), binpttrue, binptsmear)->Write();
    }
  }
  for(auto binshapetrue : ROOT::TSeqI(0, h2true->GetXaxis()->GetNbins())){
    for(auto binshapesmear : ROOT::TSeqI(0, h2smeared->GetXaxis()->GetNbins())){
      sliceRepsonsePt(response, observable.data(), binshapetrue, binshapesmear)->Write();
    }
  }

  for(auto u : unfoldingresult) {
    std::string dirname(Form("iteration%d", std::get<0>(u)));
    fout->mkdir(dirname.data());
    fout->cd(dirname.data());

    std::get<1>(u)->Write();                        // Unfolded
    std::get<2>(u)->Write();                        // Refolded
    std::get<3>(u)->Write();                        // Unfolded, MC closure
    std::get<4>(u)->Write();                        // Unfolded, MC self-closure
    std::get<5>(u)->Write();                        // Refolded, MC closure
    std::get<6>(u)->Write();                        // Refolded, MC self-closure
    for(auto m : std::get<7>(u)) m->Write();        // Correlation shape
    for(auto m : std::get<8>(u)) m->Write();        // Correlation pt
  }
}