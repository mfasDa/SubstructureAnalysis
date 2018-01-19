#ifndef __CLING__
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "Rtypes.h"
#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"
#include "TGaxis.h"
#include "TGraphErrors.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TMath.h"
#include "TObjArray.h"
#include "TStopwatch.h"
#include "TString.h"
#include "TSystemFile.h"
#include "TSystemDirectory.h"
#include "TTree.h"
#include <TVector3.h>
#endif

//DEFINITION OF A FEW CONSTANTS
const Float_t ptmin = 0.0;
const Float_t ptmax = 50.0;
const Float_t phimin = 0.;
const Float_t phimax = 2. * TMath::Pi();
const Float_t etamin = -1.;
const Float_t etamax = 1.;

void plot_PWG4HighPtTrackQAHybrid2013_Phi(const char *ContainerInputMerged = "outputAliPWG4HighPtSpectra.root", Int_t cent = 10, Int_t trig = 0, Bool_t bESD = kFALSE, Int_t run = 0, Bool_t bScaleNEvt = kTRUE)
{

  /*
    trig
    0: minimum bias
    1: central
    2: semi central
    3: MB+Central+SemiCentral
    4: EMCJE
    5: kINT7
   */

  //  gStyle->SetOptStat(111111);

  TString strTrigger = "";
  if (trig == 0)
    strTrigger = "kMB";
  if (trig == 1)
    strTrigger = "kCentral";
  if (trig == 2)
    strTrigger = "kSemiCentral";
  if (trig == 3)
    strTrigger = "kMBkCentralkSemiCentral";
  if (trig == 4)
    strTrigger = "kEMCEJE";
  if (trig == 5)
    strTrigger = "kINT7";

  TString strTrigger2 = "";
  if (trig == 0)
    strTrigger2 = "kMB";
  if (trig == 1)
    strTrigger2 = "kCentral";
  if (trig == 2)
    strTrigger2 = "kSemiCentral";
  if (trig == 3)
    strTrigger2 = "kMBkCentralSemiCentral";
  if (trig == 4)
    strTrigger2 = "kEMCEJE";
  if (trig == 5)
    strTrigger2 = "kINT7";

  Int_t globStTrackType = 0;
  Int_t globStCuts = 5;
  Int_t globCnoSPDTrackType = 7;
  Int_t globCnoSPDCuts = 5;
  Int_t globCnoITSTrackType = 7;
  Int_t globCnoITSCuts = 2;

  std::unique_ptr<TFile> f1(TFile::Open(ContainerInputMerged));
  f1->ls();
  f1->cd();

  //Load histograms
  auto NEventsGlobAll = 1.;
  auto histsGlobAll = (TList *)f1->Get(Form("PWG4_HighPtTrackQACent%dTrackType%dCuts%d%s/qa_histsQAtrackCent%dType%dcuts%d%s", cent, 0, 0, strTrigger.Data(), cent, 0, 0, strTrigger2.Data()));
  if (histsGlobAll)
  {
    auto fNEventSelGlobAll = static_cast<TH1 *>(histsGlobAll->FindObject("fNEventSel"));
    NEventsGlobAll = fNEventSelGlobAll->GetEntries();
  }

  auto histsGlobSt = (TList *)f1->Get(Form("PWG4_HighPtTrackQACent%dTrackType%dCuts%d%s/qa_histsQAtrackCent%dType%dcuts%d%s", cent, globStTrackType, globStCuts, strTrigger.Data(), cent, globStTrackType, globStCuts, strTrigger2.Data()));
  auto fNEventSelGlobSt = static_cast<TH1 *>(histsGlobSt->FindObject("fNEventSel"));
  auto NEventsGlobSt = fNEventSelGlobSt->GetEntries();

  auto histsGlobCnoSPD = (TList *)f1->Get(Form("PWG4_HighPtTrackQACent%dTrackType%dCuts%d%s/qa_histsQAtrackCent%dType%dcuts%d%s", cent, globCnoSPDTrackType, globCnoSPDCuts, strTrigger.Data(), cent, globCnoSPDTrackType, globCnoSPDCuts, strTrigger2.Data()));
  auto fNEventSelGlobCnoSPD = static_cast<TH1 *>(histsGlobCnoSPD->FindObject("fNEventSel"));
  auto NEventsGlobCnoSPD = fNEventSelGlobCnoSPD->GetEntries();

  auto NEventsGlobCnoITS = 1.;
  auto histsGlobCnoITS = (TList *)f1->Get(Form("PWG4_HighPtTrackQACent%dTrackType%dCuts%d%s/qa_histsQAtrackCent%dType%dcuts%d%s", cent, globCnoITSTrackType, globCnoITSCuts, strTrigger.Data(), cent, globCnoITSTrackType, globCnoITSCuts, strTrigger2.Data()));
  if (histsGlobCnoITS)
  {
    histsGlobCnoITS->Print();
    auto fNEventSelGlobCnoITS = static_cast<TH1 *>(histsGlobCnoITS->FindObject("fNEventSel"));
    NEventsGlobCnoITS = fNEventSelGlobCnoITS->GetEntries();
  }
  std::cout << "NEventsGlobSt: " << NEventsGlobSt << std::endl;

  if (!bScaleNEvt)
  {
    NEventsGlobAll = 1.;
    NEventsGlobSt = 1.;
    NEventsGlobCnoSPD = 1.;
  }

  TH2 *fPtPhiGlobAll(nullptr);
  if (histsGlobAll)
    fPtPhiGlobAll = static_cast<TH2 *>(histsGlobAll->FindObject("fPtPhi"));
  auto fPtPhiGlobSt = static_cast<TH2 *>(histsGlobSt->FindObject("fPtPhi"));
  auto fPtPhiGlobCnoSPD = static_cast<TH2 *>(histsGlobCnoSPD->FindObject("fPtPhi"));
  TH2 *fPtPhiGlobCnoITS(nullptr);
  if (histsGlobCnoITS)
    fPtPhiGlobCnoITS = static_cast<TH2 *>(histsGlobCnoITS->FindObject("fPtPhi"));
  if (histsGlobAll)
  {
    fPtPhiGlobAll->SetXTitle("p_{T} [GeV]");
    fPtPhiGlobAll->SetYTitle("#phi");
  }
  fPtPhiGlobSt->SetXTitle("p_{T} [GeV]");
  fPtPhiGlobSt->SetYTitle("#phi");
  fPtPhiGlobCnoSPD->SetXTitle("p_{T} [GeV]");
  fPtPhiGlobCnoSPD->SetYTitle("#phi");

  if (histsGlobAll)
    fPtPhiGlobAll->Scale(1. / NEventsGlobAll, "width");
  if (histsGlobCnoITS)
    fPtPhiGlobCnoITS->Scale(1. / NEventsGlobCnoITS, "width");

  auto c1 = new TCanvas("c1", "c1: Pt Phi", 600, 400);
  c1->Divide(2, 2);
  c1->cd(1);
  if (histsGlobAll)
  {
    //    fPtPhiGlobAll->Scale(1./NEventsGlobAll,"width");
    fPtPhiGlobAll->Draw("colz");
  }
  c1->cd(3);
  //gPad->SetLogz();
  fPtPhiGlobSt->Scale(1. / NEventsGlobSt, "width");
  fPtPhiGlobSt->Draw("colz");
  c1->cd(4);
  fPtPhiGlobCnoSPD->Scale(1. / NEventsGlobCnoSPD, "width");
  fPtPhiGlobCnoSPD->Draw("colz");

  c1->SaveAs(Form("PtPhiCent%d.png", cent));

  auto c2 = new TCanvas("c2", "c2: Phi", 600, 450);
  Int_t binMin = 1;
  if (ptmin > 0.)
    binMin = fPtPhiGlobSt->GetXaxis()->FindBin(ptmin);
  std::cout << "binMin: " << binMin << "  " << fPtPhiGlobSt->GetXaxis()->GetBinLowEdge(binMin) << std::endl;
  auto fPhiGlobSt = fPtPhiGlobSt->ProjectionY("fPhiGlobSt", binMin, fPtPhiGlobSt->GetNbinsX());
  auto fPhiGlobCnoSPD = fPtPhiGlobCnoSPD->ProjectionY("fPhiGlobCnoSPD", binMin, fPtPhiGlobSt->GetNbinsX());

  TH1 *fPhiGlobAll(nullptr);
  if (histsGlobAll)
  {
    fPhiGlobAll = fPtPhiGlobAll->ProjectionY("fPhiGlobAll", binMin, fPtPhiGlobAll->GetNbinsX());
    fPhiGlobAll->SetLineColor(4);
    fPhiGlobAll->SetLineWidth(3);
  }
  fPhiGlobSt->SetLineColor(2);
  fPhiGlobSt->SetLineWidth(3);

  fPhiGlobCnoSPD->SetLineStyle(1);
  fPhiGlobCnoSPD->SetLineColor(6);
  fPhiGlobCnoSPD->SetLineWidth(3);

  TH1 *fPhiGlobCnoITS(nullptr);
  if (histsGlobCnoITS)
  {
    Printf("plot noITSrefit phi dist");
    fPhiGlobCnoITS = fPtPhiGlobCnoITS->ProjectionY("fPhiGlobCnoITS", binMin, fPtPhiGlobSt->GetNbinsX());
    fPhiGlobCnoITS->SetLineStyle(1);
    fPhiGlobCnoITS->SetLineColor(8);
    fPhiGlobCnoITS->SetLineWidth(3);
  }

  auto fPhiGlobSum = static_cast<TH1 *>(fPhiGlobSt->Clone());
  fPhiGlobSum->SetTitle("fPhiGlobSum");
  fPhiGlobSum->SetName("fPhiGlobSum");
  fPhiGlobSum->Add(fPhiGlobCnoSPD);

  gPad->SetLeftMargin(0.2);
  gPad->SetRightMargin(0.02);
  gPad->SetBottomMargin(0.12);

  auto frame2 = gPad->DrawFrame(0., 0., 2. * TMath::Pi(), fPhiGlobSum->GetBinContent(fPhiGlobSum->GetMaximumBin()) * 2.); //1.5);
  frame2->SetXTitle("#phi");
  frame2->SetYTitle("#frac{1}{N_{evts}}#frac{dN}{d#phi}");
  frame2->GetXaxis()->SetTitleSize(0.06);
  frame2->GetYaxis()->SetTitleSize(0.06);
  frame2->GetXaxis()->SetTitleOffset(0.75);
  frame2->GetYaxis()->SetTitleOffset(1.4);

  // fPhiGlobAll->DrawCopy("same");

  fPhiGlobSt->DrawCopy("same");
  fPhiGlobCnoSPD->DrawCopy("same");

  TH1 *fPhiGlobSum2(nullptr);
  if (histsGlobCnoITS)
  {
    fPhiGlobCnoITS->DrawCopy("same");
    fPhiGlobSum2 = static_cast<TH1 *>(fPhiGlobSt->Clone());
    fPhiGlobSum2->SetTitle("fPhiGlobSum2");
    fPhiGlobSum2->SetName("fPhiGlobSum2");
    fPhiGlobSum2->Add(fPhiGlobCnoITS);
    fPhiGlobSum2->SetLineColor(4);
    fPhiGlobSum2->SetMarkerColor(4);
    fPhiGlobSum2->DrawCopy("same");
  }

  fPhiGlobSum->SetLineColor(1);
  fPhiGlobSum->SetMarkerColor(1);
  fPhiGlobSum->DrawCopy("same");

  auto leg2 = new TLegend(0.22, 0.6, 0.88, 0.88, Form("Hybrid tracks. run:%d", run));
  leg2->SetFillColor(10);
  leg2->SetBorderSize(0);
  leg2->SetFillStyle(0);
  leg2->SetTextSize(0.06);
  leg2->AddEntry(fPhiGlobSt, "w/ SPD & ITSrefit", "l");
  leg2->AddEntry(fPhiGlobCnoSPD, "w/o SPD & w/ ITSrefit", "l");
  leg2->AddEntry(fPhiGlobSum, "sum", "l");
  if (histsGlobCnoITS)
  {
    leg2->AddEntry(fPhiGlobCnoITS, "w/o SPD || w/o ITSrefit", "l");
    leg2->AddEntry(fPhiGlobSum2, "Incl w/o ITSrefit", "l");
  }
  leg2->Draw();

  //NEventsGlobSt
  TLatex textNEvents;
  textNEvents.SetNDC();
  textNEvents.DrawLatex(0.52, 0.42, Form("#it{N}_{events} = %.0f", NEventsGlobSt));

  c2->SaveAs(Form("PhiCent%d%sRun%d.png", cent, strTrigger.Data(), run));
  c2->SaveAs(Form("PhiCent%d%sRun%d.eps", cent, strTrigger.Data(), run));

  auto c3 = new TCanvas("c3", "c3: Pt spectra", 600, 450);

  gPad->SetLeftMargin(0.2);
  gPad->SetRightMargin(0.02);
  gPad->SetBottomMargin(0.12);

  auto frame3 = gPad->DrawFrame(0., 1e-7, 100., 1e5);
  frame3->SetXTitle("p_{T} [GeV]");
  frame3->SetYTitle("#frac{1}{N_{evts}}#frac{dN}{dp_{T}} [GeV^{-1}]");
  frame3->GetXaxis()->SetTitleSize(0.06);
  frame3->GetYaxis()->SetTitleSize(0.06);
  frame3->GetXaxis()->SetTitleOffset(0.75);
  frame3->GetYaxis()->SetTitleOffset(1.4);

  gPad->SetLogy();

  auto fPtGlobSt = fPtPhiGlobSt->ProjectionX("fPtGlobSt");
  auto fPtGlobCnoSPD = fPtPhiGlobCnoSPD->ProjectionX("fPtGlobCnoSPD");
  TH1 *fPtGlobAll(nullptr);
  if (histsGlobAll)
    fPtGlobAll = fPtPhiGlobAll->ProjectionX("fPtGlobAll");

  fPtGlobSt->SetLineColor(2);
  fPtGlobSt->SetLineWidth(3);

  if (histsGlobAll)
  {
    fPtGlobAll->SetLineColor(4);
    fPtGlobAll->SetLineWidth(3);
    //    fPtGlobAll->DrawCopy("same");
  }

  fPtGlobCnoSPD->SetLineStyle(1);
  fPtGlobCnoSPD->SetLineColor(8);
  fPtGlobCnoSPD->SetLineWidth(3);

  fPtGlobSt->DrawCopy("same");
  fPtGlobCnoSPD->DrawCopy("same");

  auto leg3 = new TLegend(0.22, 0.6, 0.88, 0.88, "Hybrid tracks");
  leg3->SetFillColor(10);
  leg3->SetBorderSize(0);
  leg3->SetFillStyle(0);
  leg3->SetTextSize(0.06);
  leg3->AddEntry(fPtGlobSt, "w/ SPD & ITSrefit", "l");
  leg3->AddEntry(fPtGlobCnoSPD, "w/o SPD & w/ ITSrefit", "l");
  //  leg3->AddEntry(fPtGlobAll,"all hybrids","l");
  leg3->Draw();

  c3->SaveAs(Form("PtCent%d.png", cent));

  std::unique_ptr<TFile> histos(TFile::Open("TrackPtSpectra.root", "RECREATE"));

  fPtGlobSt->Write("fPtGlobSt");
  fPtGlobCnoSPD->Write("fPtGlobCnoSPD");

  histos->Write();
  histos->Close();
}
