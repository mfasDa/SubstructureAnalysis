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
#include <iostream>
#include <cstdlib>
#include "TRandom.h"
#include "TH1D.h"
#include "RStringView.h"

#include "TFile.h"
#include "TVectorD.h"

#include "TROOT.h"
#include "ROOT/TDataFrame.hxx"
#include "ROOT/TSeq.hxx"
#include "TString.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TRandom.h"
#include "TPostScript.h"
#include "TH2D.h"
#include "TFile.h"
#include "TLine.h"
#include "TNtuple.h"
#include "TMath.h"
#include "TProfile.h"
#include "TTreeReader.h"

#include "RooUnfoldResponse.h"
#include "RooUnfoldBayes.h"
//#include "RooUnfoldTestHarness2D.h"
#endif

TH2D *CorrelationHistShape(const TMatrixD &cov, const char *name, const char *title,
                           Int_t na, Int_t nb, Int_t kbin);
TH2D *CorrelationHistPt(const TMatrixD &cov, const char *name, const char *title,
                        Int_t na, Int_t nb, Int_t kbin);
void Normalize2D(TH2 *h);
TH1D *TruncateHisto(TH1D *gr, Int_t nbinsold, Int_t lowold, Int_t highold, Int_t nbinsnew, Int_t lownew, Int_t highnew, Int_t lim);

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

void RooSimpleAngu(std::string_view filedata, std::string_view filemc)
{
#if defined (__CINT__) && !defined(__CLING__)
  gSystem->Load("libRooUnfold");
#endif
  Int_t difference = 1;
  Int_t Ppol = 0;
  std::cout << "==================================== pick up the response matrix for background==========================" << std::endl;
  ///////////////////parameter setting
  RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovariance;

  Double_t ptbins[] =  {0., 20., 30., 40., 50., 60., 80., 100., 120., 140., 160., 180., 200., 250., 300., 400.};
  // angularity must range out up to 0.2
  Double_t angularitybins[] = {0., 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09, 0.1, 0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.2};

  const Int_t kNPtBinsTrue = 20,
              kNAngBinsSmearedNC = 20;

  TH2D *hraw(new TH2D("hraw", "hraw", sizeof(angularitybins)/sizeof(double)-1, angularitybins, sizeof(ptbins)/sizeof(double)-1, ptbins)),
       *h2smeared(new TH2D("smeared", "smeared", sizeof(angularitybins)/sizeof(double)-1, angularitybins, sizeof(ptbins)/sizeof(double)-1, ptbins)), //detector measured level but no cuts
       *h2smearednocuts(new TH2D("smearednocuts", "smearednocuts", kNAngBinsSmearedNC, 0., 0.2, sizeof(ptbins)/sizeof(double)-1, ptbins)),  //true correlations with measured cuts
       *h2true(new TH2D("true", "true", sizeof(angularitybins)/sizeof(double)-1, angularitybins, sizeof(ptbins)/sizeof(double)-1, ptbins)),   //full true correlation
       *h2fulleff(new TH2D("truef", "truef", sizeof(angularitybins)/sizeof(double)-1, angularitybins, sizeof(ptbins)/sizeof(double)-1, ptbins)),
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

  //////////GET THE DATA////////////
  std::unique_ptr<TFile> datafilereader(TFile::Open(filedata.data(), "READ"));
  TTreeReader datareader(static_cast<TTree *>(datafilereader->Get("jetSubstructure")));
  TTreeReaderValue<double>  ptrecData(datareader, "PtJetRec"), 
                            angularityRecData(datareader, "AngularityMeasured");
  for(auto en : datareader){
    if(*ptrecData < 20 || *ptrecData > 200 || *angularityRecData < 0 || *angularityRecData > 0.2) continue;
    hraw->Fill(*angularityRecData, *ptrecData);
  }

  ////Get the MC////////////////////////
  // TDataFrame not supported in ROOUnfold (yet) - needs TTreeTreader
  // can however be done with Reduce function

  std::unique_ptr<TFile> mcfilereader(TFile::Open(filemc.data(), "READ"));
  TTreeReader mcreader(static_cast<TTree *>(mcfilereader->Get("jetSubstructureMerged")));
  TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                            ptsim(mcreader, "PtJetSim"), 
                            angularityRec(mcreader, "AngularityMeasured"), 
                            angularitySim(mcreader, "AngularityTrue"),
                            weight(mcreader, "PythiaWeight");
  for(auto en : mcreader){
    //if(*ptsim > 200.) continue;
    h2fulleff->Fill(*angularitySim, *ptsim, *weight);
    h2smearednocuts->Fill(*angularityRec, *ptrec, *weight);
    responsenotrunc.Fill(*angularityRec, *ptrec, *angularitySim, *ptsim, *weight);
    if(*ptrec > 200. || *ptrec < 20 || *angularityRec > 0.2) continue;
    h2smeared->Fill(*angularityRec, *ptrec, *weight);
    h2true->Fill(*angularitySim, *ptsim, *weight);
    response.Fill(*angularityRec, *ptrec, *angularitySim, *ptsim, *weight);
  }

  /////////COMPUTE KINEMATIC EFFICIENCIES////////////////////

  TH1F *htrueptd = (TH1F *)h2fulleff->ProjectionX("trueptd", 1, -1);
  TH1F *htruept = (TH1F *)h2fulleff->ProjectionY("truept", 1, -1);

  TH2D *hfold = (TH2D *)hraw->Clone("hfold");
  hfold->Sumw2();
  TH1D *effok = (TH1D *)h2true->ProjectionX("effok", 2, 2);
  TH1D *effok1 = (TH1D *)h2fulleff->ProjectionX("effok2", 2, 2);
  effok->Divide(effok1);
  effok->SetName("correff20-40");

  TH1D *effok3 = (TH1D *)h2true->ProjectionX("effok3", 3, 3);
  TH1D *effok4 = (TH1D *)h2fulleff->ProjectionX("effok4", 3, 3);
  effok3->Divide(effok4);
  effok3->SetName("correff40-60");

  TH1D *effok5 = (TH1D *)h2true->ProjectionX("effok5", 4, 4);
  TH1D *effok6 = (TH1D *)h2fulleff->ProjectionX("effok6", 4, 4);
  effok5->Divide(effok6);
  effok5->SetName("correff60-80");

  TString tag(filedata);
  tag.ReplaceAll(".root", "");
  TFile *fout = new TFile(Form("%s_unfolded_angularity.root", tag.Data()), "RECREATE");
  fout->cd();
  effok->Write();
  effok3->Write();
  effok5->Write();

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
    TH2D *hunf = (TH2D *)unfold.Hreco(errorTreatment);

    //FOLD BACK

    for (auto i : ROOT::TSeqI(0,sizeof(angularitybins)/sizeof(double)-1)) // bins angularity, reconstructed
    {
      for (auto j : ROOT::TSeqI(0, sizeof(ptbins)/sizeof(double)-1)) // bins pt, reconstructed
      {
        int indexmeas = i + (sizeof(angularitybins)/sizeof(double)-1) * j;
        double effects = 0;
        double error = 0;
        for (auto k : ROOT::TSeqI(0,sizeof(angularitybins)/sizeof(double)-1)) // bins angularity, true
        {
          for (auto l : ROOT::TSeqI(0, sizeof(ptbins)/sizeof(double)-1)) // bins pt, true
          {

            int indextrue = k + (sizeof(angularitybins)/sizeof(double)-1) * l;

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

    for (auto k : ROOT::TSeqI(0,sizeof(angularitybins)/sizeof(double)-1)) // bins angularity, true 
    {
      for (auto l : ROOT::TSeqI(0,sizeof(ptbins)/sizeof(double)-1)) // bins pt, true
      {

        double effects = 0;
        for (auto i : ROOT::TSeqI(0,sizeof(angularitybins)/sizeof(double)-1)) // bins angularity, reconstructed
        {
          for (auto j : ROOT::TSeqI(0, sizeof(ptbins)/sizeof(double)-1)) // bins pt, reconstructed
          {

            int indexm = i + (sizeof(angularitybins)/sizeof(double)-1) * j;
            int indext = k + (sizeof(angularitybins)/sizeof(double)-1) * l;

            effects += response(indexm, indext);
          }
        }
      }
    }

    TH2D *htempUnf = (TH2D *)hunf->Clone("htempUnf");
    htempUnf->SetName(Form("angularity_unfolded_iter%d.root", niter));

    TH2D *htempFold = (TH2D *)hfold->Clone("htempFold");
    htempFold->SetName(Form("angularity_folded_iter%d.root", niter));

    htempUnf->Write();
    htempFold->Write();

    ///HERE I GET THE COVARIANCE MATRIX/////

    if (niter == 4)
    {

      TMatrixD covmat = unfold.Ereco((RooUnfold::ErrorTreatment)RooUnfold::kCovariance);
      for (Int_t k = 0; k < h2true->GetNbinsX(); k++)
      {
        TH2D *hCorr = (TH2D *)CorrelationHistShape(covmat, Form("corr%d", k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k);
        TH2D *covshape = (TH2D *)hCorr->Clone("covshape");
        covshape->SetName(Form("pearsonmatrix_iter%d_binshape%d", niter, k));
        covshape->SetDrawOption("colz");
        covshape->Write();
      }

      for (Int_t k = 0; k < h2true->GetNbinsY(); k++)
      {
        TH2D *hCorr = (TH2D *)CorrelationHistPt(covmat, Form("corr%d", k), "Covariance matrix", h2true->GetNbinsX(), h2true->GetNbinsY(), k);
        TH2D *covpt = (TH2D *)hCorr->Clone("covpt");
        covpt->SetName(Form("pearsonmatrix_iter%d_binpt%d", niter, k));
        covpt->SetDrawOption("colz");
        covpt->Write();
      }
    }
  }
}

TH2D *CorrelationHistShape(const TMatrixD &cov, const char *name, const char *title,
                           Int_t na, Int_t nb, Int_t kbin)
{

  TH2D *h = new TH2D(name, title, nb, 0, nb, nb, 0, nb);

  for (int l = 0; l < nb; l++)
  {
    for (int n = 0; n < nb; n++)
    {
      int index1 = kbin + na * l;
      int index2 = kbin + na * n;
      Double_t Vv = cov(index1, index1) * cov(index2, index2);
      if (Vv > 0.0)
        h->SetBinContent(l + 1, n + 1, cov(index1, index2) / sqrt(Vv));
    }
  }
  return h;
}

TH2D *CorrelationHistPt(const TMatrixD &cov, const char *name, const char *title,
                        Int_t na, Int_t nb, Int_t kbin)
{

  TH2D *h = new TH2D(name, title, na, 0, na, na, 0, na);

  for (int l = 0; l < na; l++)
  {
    for (int n = 0; n < na; n++)
    {

      int index1 = l + na * kbin;
      int index2 = n + na * kbin;
      Double_t Vv = cov(index1, index1) * cov(index2, index2);
      if (Vv > 0.0)
        h->SetBinContent(l + 1, n + 1, cov(index1, index2) / sqrt(Vv));
    }
  }
  return h;
}

void Normalize2D(TH2 *h)
{
  Int_t nbinsYtmp = h->GetNbinsY();
  const Int_t nbinsY = nbinsYtmp;
  TArrayD norm(nbinsY);
  for (Int_t biny = 1; biny <= nbinsY; biny++)
  {
    norm[biny - 1] = 0;
    for (Int_t binx = 1; binx <= h->GetNbinsX(); binx++)
    {
      norm[biny - 1] += h->GetBinContent(binx, biny);
    }
  }

  for (Int_t biny = 1; biny <= nbinsY; biny++)
  {
    for (Int_t binx = 1; binx <= h->GetNbinsX(); binx++)
    {
      if (norm[biny - 1] == 0)
        continue;
      else
      {
        h->SetBinContent(binx, biny, h->GetBinContent(binx, biny) / norm[biny - 1]);
        h->SetBinError(binx, biny, h->GetBinError(binx, biny) / norm[biny - 1]);
      }
    }
  }
}

TH1D *TruncateHisto(TH1D *gr, Int_t nbinsold, Int_t lowold, Int_t highold, Int_t nbinsnew, Int_t lownew, Int_t highnew, Int_t lim)
{
  TH1D *hTruncate = new TH1D("hTruncate", "", nbinsnew, lownew, highnew);
  hTruncate->Sumw2();
  for (Int_t binx = 1; binx <= hTruncate->GetNbinsX(); binx++)
  {
    hTruncate->SetBinContent(binx, gr->GetBinContent(lim + binx));
    hTruncate->SetBinError(binx, gr->GetBinError(lim + binx));
  }

  return hTruncate;
}

#if !defined(__CLING__) 
int main(int argc, const char **argv)
{
  RooSimpleAngu(argv[0], argv[1]);
  return EXIT_SUCCESS;
} // Main program when run stand-alone
#endif
