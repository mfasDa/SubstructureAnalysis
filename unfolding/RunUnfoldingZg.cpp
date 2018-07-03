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
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "RStringView.h"
#include "ROOT/TSeq.hxx"
#include "ROOT/TProcessExecutor.hxx"
#include "TH2D.h"
#include "TFile.h"
#include "TKey.h"
#include "TROOT.h"
#include "TTreeReader.h"

#include "RooUnfoldResponse.h"
#include "RooUnfoldBayes.h"
//#include "RooUnfoldTestHarness2D.h"
#endif

#include "../helpers/filesystem.C"
#include "../helpers/string.C"
#include "../helpers/unfolding.C"

//==============================================================================
// Global definitions
//==============================================================================

//const Double_t cutdummy = -99999.0;

//==============================================================================
// Gaussian smearing, systematic translation, and variable inefficiency
//==============================================================================

//==============================================================================
// Example Unfolding
//==============================================================================

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

void RunUnfoldingZg(const std::string_view filedata, const std::string_view filemc)
{
  ROOT::EnableThreadSafety();
  Int_t difference = 1;
  Int_t Ppol = 0;
  std::cout << "==================================== pick up the response matrix for background==========================" << std::endl;
  ///////////////////parameter setting
  RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovariance;

  auto ptbinvec_smear = MakePtBinningSmeared(filedata); // Smeared binnning - only in the region one trusts the data
  std::vector<double> ptbinvec_true = {0., 20., 40., 60., 80., 100., 120., 140., 160., 180., 200., 220., 240., 280., 320., 360., 400.}; // True binning, needs overlap to over/underflow bins
  // zg must range from 0 to 0.5
  Double_t zgbins[] = {0., 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5};

  const Int_t kNZgBinsSmearedNC = 10;

  TH2D *hraw(new TH2D("hraw", "hraw", sizeof(zgbins)/sizeof(double)-1, zgbins, ptbinvec_smear.size() - 1, ptbinvec_smear.data())),
       *h2smeared(new TH2D("smeared", "smeared", sizeof(zgbins)/sizeof(double)-1, zgbins, ptbinvec_smear.size()-1, ptbinvec_smear.data())), //detector measured level but no cuts
       *h2smearednocuts(new TH2D("smearednocuts", "smearednocuts",sizeof(zgbins)/sizeof(double)-1,zgbins, ptbinvec_true.size()-1, ptbinvec_true.data())),  //true correlations with measured cuts
       *h2true(new TH2D("true", "true", sizeof(zgbins)/sizeof(double)-1, zgbins, ptbinvec_true.size()-1, ptbinvec_true.data())),   //full true correlation
       *h2fulleff(new TH2D("truef", "truef", sizeof(zgbins)/sizeof(double)-1, zgbins, ptbinvec_true.size()-1, ptbinvec_true.data())),
       *hcovariance(new TH2D("covariance", "covariance", 10, 0., 1., 10, 0, 1.));

  hraw->Sumw2();
  h2smeared->Sumw2();
  h2true->Sumw2();
  h2fulleff->Sumw2();
  h2smearednocuts->Sumw2();

  RooUnfoldResponse response;
  RooUnfoldResponse responsenotrunc;
  response.Setup(h2smeared, h2true);
  responsenotrunc.Setup(h2smearednocuts, h2fulleff);

  // define reconstruction level cuts
  auto smearptmin = *(std::min_element(ptbinvec_smear.begin(), ptbinvec_smear.end()));
  auto smearptmax = *(std::max_element(ptbinvec_smear.begin(), ptbinvec_smear.end()));


  /****
   * Reading data and MC in parallel
   * Usign one thread for data processing and one thread for MC processing
   */

  //////////GET THE DATA////////////
  std::thread datathread([&]() {
    std::cout << "Datathread: Fill histograms from data" << std::endl;
    std::unique_ptr<TFile> datafilereader(TFile::Open(filedata.data(), "READ"));
    TTreeReader datareader(GetDataTree(*datafilereader));
    TTreeReaderValue<double>  ptrecData(datareader, "PtJetRec"), 
                              zgRecData(datareader, "ZgMeasured");
    for(auto en : datareader){
      if(*ptrecData < smearptmin || *ptrecData > smearptmax) continue;
      hraw->Fill(*zgRecData, *ptrecData);
    }
    std::cout << "Datathread: Data ready" << std::endl;
  });

  ////Get the MC////////////////////////
  std::thread mcthread([&](){
    // TDataFrame not supported in ROOUnfold (yet) - needs TTreeTreader
    // can however be done with Reduce function
    std::cout << "MCthread: Fill histograms from simulation" << std::endl;
    std::unique_ptr<TFile> mcfilereader(TFile::Open(filemc.data(), "READ"));
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
      if(*ptrec > smearptmax || *ptrec < smearptmin) continue;
      h2smeared->Fill(*zgRec, *ptrec, *weight);
      h2true->Fill(*zgSim, *ptsim, *weight);
      response.Fill(*zgRec, *ptrec, *zgSim, *ptsim, *weight);
    }
    std::cout << "MCthread: Response ready" << std::endl;
  });
  datathread.join();
  mcthread.join();
  //return;

  auto htrueptd = h2fulleff->ProjectionX("trueptd", 1, -1);
  auto htruept = h2fulleff->ProjectionY("truept", 1, -1);

  // template for folded spectrum

  /////////COMPUTE KINEMATIC EFFICIENCIES////////////////////

  std::vector<TH1 *> efficiencies;
  for(auto b : ROOT::TSeqI(1, h2true->GetYaxis()->GetNbins()+1)){
    // ignore bins which are outside the kinematic range of the measurement
    if(h2true->GetYaxis()->GetBinUpEdge(b) < smearptmin || h2true->GetYaxis()->GetBinLowEdge(b) > smearptmax) continue;
    std::unique_ptr<TH1> truncated(h2true->ProjectionX("truncated", b, b)), 
                         full(h2fulleff->ProjectionX("full", b, b));
    TH1 *efficiency = static_cast<TH1 *>(truncated->Clone(Form("efficiency_%d_%d", int(h2true->GetYaxis()->GetBinLowEdge(b)), int(h2true->GetYaxis()->GetBinUpEdge(b)))));
    efficiency->Sumw2();
    efficiency->SetDirectory(nullptr);
    efficiency->Divide(full.get());
    efficiencies.emplace_back(efficiency);
  }

  using resultformat = std::tuple<int, TH2 *, TH2 *, std::vector<TH2 *>, std::vector<TH2 *>>;
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
      hunf->SetName(Form("zg_unfolded_iter%d.root", niter));

      // FOLD BACK
      auto hfold = Refold(hraw, hunf, response);
      hfold->SetName(Form("zg_folded_iter%d.root", niter));

      //CheckNormalized(response, sizeof(zgbins)/sizeof(double)-1, sizeof(zgbins)/sizeof(double)-1, ptbinvec_true.size()-1, ptbinvec_smear.size()-1);

      TMatrixD covmat = unfold.Ereco((RooUnfold::ErrorTreatment)RooUnfold::kCovariance);
      std::vector<TH2 *> shapematrices, ptmatrices;
      for (auto k : ROOT::TSeqI(0, h2true->GetNbinsX()))
        shapematrices.emplace_back(CorrelationHistShape(covmat, Form("pearsonmatrix_iter%d_binshape%d", niter, k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k));

      for (auto k : ROOT::TSeqI(0, h2true->GetNbinsY()))
        ptmatrices.emplace_back(CorrelationHistPt(covmat, Form("pearsonmatrix_iter%d_binpt%d", niter, k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k));

      result.emplace_back(std::make_tuple(niter, hunf, hfold, shapematrices, ptmatrices));
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

  ROOT::TProcessExecutor pool(NWORKERS);
  auto unfoldingresult = pool.MapReduce(workitem, ROOT::TSeqI(0, NWORKERS), reducer);

  auto tag = basename(filedata);
  tag.replace(tag.find(".root"), 5, "");
  std::unique_ptr<TFile> fout(TFile::Open(Form("%s_unfolded_zg.root", tag.data()), "RECREATE"));
  fout->cd();
  
  for(auto e : efficiencies) e->Write();

  hraw->Write();

  h2smeared->SetName("smeared");
  h2smeared->Write();
  h2true->SetName("true");
  h2true->Write();
  h2fulleff->SetName("truefull");
  h2fulleff->Write();

  for(auto u : unfoldingresult) {
    std::string dirname(Form("iteration%d", std::get<0>(u)));
    fout->mkdir(dirname.data());
    fout->cd(dirname.data());

    std::get<1>(u)->Write();                        // Unfolded
    std::get<2>(u)->Write();                        // Refolded
    for(auto m : std::get<3>(u)) m->Write();        // Response shape
    for(auto m : std::get<4>(u)) m->Write();        // Response pt
  }
}

#if !defined(__CLING__) 
int main(int argc, const char **argv)
{
  RunUnfoldingZg(argv[0], argv[1]);
  return EXIT_SUCCESS;
} // Main program when run stand-alone
#endif
