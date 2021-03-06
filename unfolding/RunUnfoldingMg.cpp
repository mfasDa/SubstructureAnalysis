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
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>

#include "RStringView.h"
#include "ROOT/TSeq.hxx"
#include "TString.h"
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
  for(auto k : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
    if(!k) continue;
    if((contains(k->GetName(), "JetSubstructure") || contains(k->GetName(), "jetSubstructure"))
       && (k->ReadObj()->IsA() == TTree::Class())) {
      result = dynamic_cast<TTree *>(k->ReadObj());
    }
  }
  std::cout << "Found tree with name " << result->GetName() << std::endl;
  return result;
}

void RunUnfoldingMg(std::string_view filedata, std::string_view filemc)
{
  Int_t difference = 1;
  Int_t Ppol = 0;
  std::cout << "==================================== pick up the response matrix for background==========================" << std::endl;
  ///////////////////parameter setting
  RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovariance;

  auto ptbinvec_smear = MakePtBinningSmeared(filedata); // Smeared binnning - only in the region one trusts the data
  std::vector<double> ptbinvec_true;
  for(auto f = 0.; f <= 400.; f+= 20.) ptbinvec_true.emplace_back(f);
  // zg must range from 0 to 0.5
  std::vector<double> massbins;
  for(auto f = 0.; f <= 50.; f+= 0.5) massbins.emplace_back(f);
  const Int_t kNZgBinsSmearedNC = 10;

  TH2D *hraw(new TH2D("hraw", "hraw", massbins.size()-1, massbins.data(), ptbinvec_smear.size() - 1, ptbinvec_smear.data())),
       *h2smeared(new TH2D("smeared", "smeared", massbins.size()-1, massbins.data(), ptbinvec_smear.size()-1, ptbinvec_smear.data())), //detector measured level but no cuts
       *h2smearednocuts(new TH2D("smearednocuts", "smearednocuts", massbins.size()-1, massbins.data(), ptbinvec_true.size()-1, ptbinvec_true.data())),  //true correlations with measured cuts
       *h2true(new TH2D("true", "true", massbins.size()-1, massbins.data(), ptbinvec_true.size()-1, ptbinvec_true.data())),   //full true correlation
       *h2fulleff(new TH2D("truef", "truef", massbins.size()-1, massbins.data(), ptbinvec_true.size()-1, ptbinvec_true.data()));

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

  //////////GET THE DATA////////////
  {
    std::unique_ptr<TFile> datafilereader(TFile::Open(filedata.data(), "READ"));
    TTreeReader datareader(GetDataTree(*datafilereader));
    TTreeReaderValue<double>  ptrecData(datareader, "PtJetRec"), 
                              massRecData(datareader, "MgMeasured");
    for(auto en : datareader){
      if(*ptrecData < smearptmin || *ptrecData > smearptmax) continue;
      hraw->Fill(*massRecData, *ptrecData);
    }
  }

  ////Get the MC////////////////////////
  // TDataFrame not supported in ROOUnfold (yet) - needs TTreeTreader
  // can however be done with Reduce function
  {
    std::unique_ptr<TFile> mcfilereader(TFile::Open(filemc.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*mcfilereader));
    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
                              massRec(mcreader, "MgMeasured"), 
                              massSim(mcreader, "MgTrue"),
                              weight(mcreader, "PythiaWeight");
    for(auto en : mcreader){
      //if(*ptsim > 200.) continue;
      h2fulleff->Fill(*massSim, *ptsim, *weight);
      h2smearednocuts->Fill(*massRec, *ptrec, *weight);
      responsenotrunc.Fill(*massRec, *ptrec, *massSim, *ptsim, *weight);

      // apply reconstruction level cuts
      if(*ptrec > smearptmax || *ptrec < smearptmin) continue;
      h2smeared->Fill(*massRec, *ptrec, *weight);
      h2true->Fill(*massSim, *ptsim, *weight);
      response.Fill(*massRec, *ptrec, *massSim, *ptsim, *weight);
    }
  }

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

  auto tag = basename(filedata);
  tag.replace(tag.find(".root"), 5, "");
  std::unique_ptr<TFile> fout(TFile::Open(Form("%s_unfolded_mg.root", tag.data()), "RECREATE"));
  fout->cd();
  
  for(auto e : efficiencies) e->Write();

  hraw->Write();

  h2smeared->SetName("smeared");
  h2smeared->Write();
  h2true->SetName("true");
  h2true->Write();
  h2fulleff->SetName("truefull");
  h2fulleff->Write();

  for (auto niter : ROOT::TSeqI(1, 35))
  {
    std::cout << "iteration" << niter << std::endl;
    std::cout << "==============Unfold h1=====================" << std::endl;

    RooUnfoldBayes unfold(&response, hraw, niter); // OR
    auto hunf = (TH2D *)unfold.Hreco(errorTreatment);
    hunf->SetName(Form("mass_unfolded_iter%d", niter));

    // FOLD BACK
    // outer loop - reconstructed
    // inner loop - true
    auto hfold = (TH2D *)hraw->Clone(Form("mass_folded_iter%d", niter));
    hfold->Sumw2();

    for (auto i : ROOT::TSeqI(0,massbins.size()-1)) // bins angularity, reconstructed
    {
      for (auto j : ROOT::TSeqI(0, ptbinvec_smear.size()-1)) // bins pt, reconstructed
      {
        int indexmeas = i + (massbins.size()-1) * j;
        double effects = 0;
        double error = 0;
        for (auto k : ROOT::TSeqI(0,massbins.size()-1)) // bins angularity, true
        {
          for (auto l : ROOT::TSeqI(0, ptbinvec_true.size()-1)) // bins pt, true
          {

            int indextrue = k + (massbins.size()-1) * l;

            //std::cerr << "indexm " << indexm << ", indext " << indext <<std::endl;

            effects += hunf->GetBinContent(k + 1, l + 1) * response(indexmeas, indextrue);
            error += TMath::Power(hunf->GetBinError(k + 1, l + 1) * response(indexmeas, indextrue), 2);
          }
        }
        hfold->SetBinContent(i + 1, j + 1, effects);
        hfold->SetBinError(i + 1, j + 1, TMath::Sqrt(error));
      }
    }

    ///////////check that the response is normalized//////////////
    // outer loop - true
    // inner loop - reconstructed

    for (auto k : ROOT::TSeqI(0,massbins.size()-1)) // bins angularity, true 
    {
      for (auto l : ROOT::TSeqI(0,ptbinvec_true.size()-1)) // bins pt, true
      {

        double effects = 0;
        for (auto i : ROOT::TSeqI(0,massbins.size()-1)) // bins angularity, reconstructed
        {
          for (auto j : ROOT::TSeqI(0, ptbinvec_smear.size()-1)) // bins pt, reconstructed
          {

            int indexm = i + (massbins.size()-1) * j;
            int indext = k + (massbins.size()-1) * l;

            effects += response(indexm, indext);
          }
        }
      }
    }

    hunf->Write();
    hfold->Write();

    ///HERE I GET THE COVARIANCE MATRIX/////

    if (niter == 4)
    {

      TMatrixD covmat = unfold.Ereco((RooUnfold::ErrorTreatment)RooUnfold::kCovariance);
      for (auto k : ROOT::TSeqI(0, h2true->GetNbinsX()))
      {
        auto covshape = CorrelationHistShape(covmat, Form("pearsonmatrix_iter%d_binshape%d", niter, k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k);
        covshape->SetDrawOption("colz");
        covshape->Write();
      }

      for (auto k : ROOT::TSeqI(0, h2true->GetNbinsY()))
      {
        auto covpt = CorrelationHistPt(covmat, Form("pearsonmatrix_iter%d_binpt%d", niter, k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k);
        covpt->SetDrawOption("colz");
        covpt->Write();
      }

    }
  }
}

#if !defined(__CLING__) 
int main(int argc, const char **argv)
{
  RunUnfoldingMg(argv[0], argv[1]);
  return EXIT_SUCCESS;
} // Main program when run stand-alone
#endif

