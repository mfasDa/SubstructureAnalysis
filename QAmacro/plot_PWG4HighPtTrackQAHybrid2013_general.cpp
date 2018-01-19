#ifndef __CLING__
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>

#include "Rtypes.h"
#include "TCanvas.h"
#include "TChain.h"
#include "TFile.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TGaxis.h"
#include "TGraphErrors.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TMath.h"
#include "TObjArray.h"
#include "TProfile.h"
#include "TStopwatch.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
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

enum whichvar
{
  kPhivspt = 0,
  kEtavspt = 1,
  kNbClsTPCvspT = 2,
  kNbClsTPCvsphi = 3,
  kNbCroRowTPCvspT = 4,
  kNbCroRowTPCvsphi = 5
};

void plot_PWG4HighPtTrackQAHybrid2013_general(const char *ContainerInputMerged = "outputAliPWG4HighPtSpectra.root", Int_t cent = 10, Int_t trig = 0, Bool_t bESD = kFALSE, Int_t run = 0, Bool_t bScaleNEvt = kTRUE, Int_t whichvar = 0)
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
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

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

  TFile *f1 = TFile::Open(ContainerInputMerged);
  f1->ls();
  f1->cd();

  //Load histograms
  float NEventsGlobAll = 1., NEventsGlobCnoITS= 1.;
  auto histsGlobAll = (TList *)f1->Get(Form("PWG4_HighPtTrackQACent%dTrackType%dCuts%d%s/qa_histsQAtrackCent%dType%dcuts%d%s", cent, 0, 0, strTrigger.Data(), cent, 0, 0, strTrigger2.Data()));
  if (histsGlobAll)
  {
    auto fNEventSelGlobAll = static_cast<TList *>(histsGlobAll->FindObject("fNEventSel"));
    NEventsGlobAll = fNEventSelGlobAll->GetEntries();
  }

  auto histsGlobSt = (TList *)f1->Get(Form("PWG4_HighPtTrackQACent%dTrackType%dCuts%d%s/qa_histsQAtrackCent%dType%dcuts%d%s", cent, globStTrackType, globStCuts, strTrigger.Data(), cent, globStTrackType, globStCuts, strTrigger2.Data()));
  auto fNEventSelGlobSt = static_cast<TH1 *>(histsGlobSt->FindObject("fNEventSel"));
  auto NEventsGlobSt = fNEventSelGlobSt->GetEntries();

  auto histsGlobCnoSPD = (TList *)f1->Get(Form("PWG4_HighPtTrackQACent%dTrackType%dCuts%d%s/qa_histsQAtrackCent%dType%dcuts%d%s", cent, globCnoSPDTrackType, globCnoSPDCuts, strTrigger.Data(), cent, globCnoSPDTrackType, globCnoSPDCuts, strTrigger2.Data()));
  auto fNEventSelGlobCnoSPD = static_cast<TH1 *>(histsGlobCnoSPD->FindObject("fNEventSel"));
  auto NEventsGlobCnoSPD = fNEventSelGlobCnoSPD->GetEntries();

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
  auto fPtPhiGlobCnoITS = static_cast<TH2 *>(histsGlobCnoITS->FindObject("fPtPhi"));
  auto fPtPhiGlobCnoSPD = static_cast<TH2 *>(histsGlobCnoSPD->FindObject("fPtPhi"));

  TH2 *fPtVarGlobAll(nullptr), *fPtVarGlobSt(nullptr), *fPtVarGlobCnoSPD(nullptr), *fPtVarGlobCnoITS(nullptr);
  if (histsGlobAll)
  {
    fPtPhiGlobAll->SetXTitle("p_{T} [GeV]");
    fPtPhiGlobAll->SetYTitle("#phi");
  }

  switch (whichvar)
  {

  case kPhivspt:
    if (histsGlobAll)
      fPtVarGlobAll = static_cast<TH2 *>(histsGlobAll->FindObject("fPtPhi"));
    fPtVarGlobSt = static_cast<TH2 *>(histsGlobSt->FindObject("fPtPhi"));
    fPtVarGlobCnoSPD = static_cast<TH2 *>(histsGlobCnoSPD->FindObject("fPtPhi"));
    if (histsGlobCnoITS)
      fPtVarGlobCnoITS = static_cast<TH2 *>(histsGlobCnoITS->FindObject("fPtPhi"));
    if (histsGlobAll)
    {
      fPtVarGlobAll->SetXTitle("p_{T} [GeV]");
      fPtVarGlobAll->SetYTitle("#phi");
    }
    fPtVarGlobSt->SetXTitle("p_{T} [GeV]");
    fPtVarGlobSt->SetYTitle("#phi");
    fPtVarGlobCnoSPD->SetXTitle("p_{T} [GeV]");
    fPtVarGlobCnoSPD->SetYTitle("#phi");
    break;

  case kEtavspt:
    if (histsGlobAll)
      fPtVarGlobAll = static_cast<TH2 *>(histsGlobAll->FindObject("fPtEta"));
    fPtVarGlobSt = static_cast<TH2 *>(histsGlobSt->FindObject("fPtEta"));
    fPtVarGlobCnoSPD = static_cast<TH2 *>(histsGlobCnoSPD->FindObject("fPtEta"));
    if (histsGlobCnoITS)
      fPtVarGlobCnoITS = static_cast<TH2 *>(histsGlobCnoITS->FindObject("fPtEta"));
    if (histsGlobAll)
    {
      fPtVarGlobAll->SetXTitle("p_{T} [GeV]");
      fPtVarGlobAll->SetYTitle("#eta");
    }
    fPtVarGlobSt->SetXTitle("p_{T} [GeV]");
    fPtVarGlobSt->SetYTitle("#eta");
    fPtVarGlobCnoSPD->SetXTitle("p_{T} [GeV]");
    fPtVarGlobCnoSPD->SetYTitle("#eta");
    break;

  case kNbClsTPCvspT:
    if (histsGlobAll)
      fPtVarGlobAll = static_cast<TH2 *>(histsGlobAll->FindObject("fPtNClustersTPC"));
    fPtVarGlobSt = static_cast<TH2 *>(histsGlobSt->FindObject("fPtNClustersTPC"));
    fPtVarGlobCnoSPD = static_cast<TH2 *>(histsGlobCnoSPD->FindObject("fPtNClustersTPC"));
    if (histsGlobCnoITS)
      fPtVarGlobCnoITS = static_cast<TH2 *>(histsGlobCnoITS->FindObject("fPtNClustersTPC"));
    if (histsGlobAll)
    {
      fPtVarGlobAll->SetXTitle("p_{T} [GeV]");
      fPtVarGlobAll->SetYTitle("Nb Cls TPC");
    }
    fPtVarGlobSt->SetXTitle("p_{T} [GeV]");
    fPtVarGlobSt->SetYTitle("Nb Cls TPC");
    fPtVarGlobCnoSPD->SetXTitle("p_{T} [GeV]");
    fPtVarGlobCnoSPD->SetYTitle("Nb Cls TPC");
    break;

  case kNbClsTPCvsphi:
    if (histsGlobAll)
      fPtVarGlobAll = static_cast<TH2 *>(histsGlobAll->FindObject("fPtNClustersTPCPhi"));
    fPtVarGlobSt = static_cast<TH2 *>(histsGlobSt->FindObject("fPtNClustersTPCPhi"));
    fPtVarGlobCnoSPD = static_cast<TH2 *>(histsGlobCnoSPD->FindObject("fPtNClustersTPCPhi"));
    if (histsGlobCnoITS)
      fPtVarGlobCnoITS = static_cast<TH2 *>(histsGlobCnoITS->FindObject("fPtNClustersTPCPhi"));
    if (histsGlobAll)
    {
      fPtVarGlobAll->SetXTitle("#phi");
      fPtVarGlobAll->SetYTitle("Nb Cls TPC");
    }
    fPtVarGlobSt->SetXTitle("#phi");
    fPtVarGlobSt->SetYTitle("Nb Cls TPC");
    fPtVarGlobCnoSPD->SetXTitle("#phi");
    fPtVarGlobCnoSPD->SetYTitle("Nb Cls TPC");
    break;

  case kNbCroRowTPCvspT:
    if (histsGlobAll)
      fPtVarGlobAll = static_cast<TH2 *>(histsGlobAll->FindObject("fPtNCrossedRows"));
    fPtVarGlobSt = static_cast<TH2 *>(histsGlobSt->FindObject("fPtNCrossedRows"));
    fPtVarGlobCnoSPD = static_cast<TH2 *>(histsGlobCnoSPD->FindObject("fPtNCrossedRows"));
    if (histsGlobCnoITS)
      fPtVarGlobCnoITS = static_cast<TH2 *>(histsGlobCnoITS->FindObject("fPtNCrossedRows"));
    if (histsGlobAll)
    {
      fPtVarGlobAll->SetXTitle("p_{T} [GeV]");
      fPtVarGlobAll->SetYTitle("Nb Crossed Rows");
    }
    fPtVarGlobSt->SetXTitle("p_{T} [GeV]");
    fPtVarGlobSt->SetYTitle("Nb Crossed Rows");
    fPtVarGlobCnoSPD->SetXTitle("p_{T} [GeV]");
    fPtVarGlobCnoSPD->SetYTitle("Nb Crossed Rows");
    break;

  case kNbCroRowTPCvsphi:

    auto fPtVarGlobAll3D = static_cast<TH3 *>(histsGlobAll->FindObject("fPtNCrossedRowsPhi")),
         fPtVarGlobSt3D = static_cast<TH3 *>(histsGlobSt->FindObject("fPtNCrossedRowsPhi")),
         fPtVarGlobCnoSPD3D = static_cast<TH3 *>(histsGlobCnoSPD->FindObject("fPtNCrossedRowsPhi")),
         fPtVarGlobCnoITS3D = static_cast<TH3 *>(histsGlobCnoITS->FindObject("fPtNCrossedRowsPhi"));

    if (histsGlobAll)
      fPtVarGlobAll = static_cast<TH2 *>(fPtVarGlobAll3D->Project3D("yz"));
    fPtVarGlobSt = static_cast<TH2 *>(fPtVarGlobSt3D->Project3D("yz"));
    fPtVarGlobCnoSPD = static_cast<TH2 *>(fPtVarGlobCnoSPD3D->Project3D("yz"));
    if (histsGlobCnoITS)
      fPtVarGlobCnoITS = static_cast<TH2 *>(fPtVarGlobCnoITS3D->Project3D("yz"));
    if (histsGlobAll)
    {
      fPtVarGlobAll->SetXTitle("#phi");
      fPtVarGlobAll->SetYTitle("Nb Crossed Rows");
    }
    fPtVarGlobSt->SetXTitle("#phi");
    fPtVarGlobSt->SetYTitle("Nb Crossed Rows");
    fPtVarGlobCnoSPD->SetXTitle("#phi");
    fPtVarGlobCnoSPD->SetYTitle("Nb Crossed Rows");
    break;
  }

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
    fPtVarGlobAll->Draw("colz");
  }
  c1->cd(3);
  //gPad->SetLogz();
  fPtVarGlobSt->Scale(1. / NEventsGlobSt, "width");
  fPtVarGlobSt->Draw("colz");
  c1->cd(4);
  fPtVarGlobCnoSPD->Scale(1. / NEventsGlobCnoSPD, "width");
  fPtVarGlobCnoSPD->Draw("colz");

  c1->SaveAs(Form("PtPhiCent%d.png", cent));

  auto c2 = new TCanvas("c2", "c2: Phi", 600, 450);
  Int_t binMin = 1;

  TH1 *fVarGlobSt(nullptr), *fVarGlobCnoSPD(nullptr);

  switch (whichvar)
  {

  case kPhivspt:
  case kEtavspt:
  case kNbClsTPCvspT:
  case kNbCroRowTPCvspT:
    if (ptmin > 0.)
      binMin = fPtVarGlobSt->GetXaxis()->FindBin(ptmin);
    std::cout << "binMin: " << binMin << "  " << fPtPhiGlobSt->GetXaxis()->GetBinLowEdge(binMin) << std::endl;
    fVarGlobSt = fPtVarGlobSt->ProjectionY("fVarGlobSt", binMin, fPtVarGlobSt->GetNbinsX());
    fVarGlobCnoSPD = fPtVarGlobCnoSPD->ProjectionY("fVarGlobCnoSPD", binMin, fPtVarGlobSt->GetNbinsX());
    break;

  case kNbClsTPCvsphi:
  case kNbCroRowTPCvsphi:
    fVarGlobSt = fPtVarGlobSt->ProfileX("_pfxGlobalSt", 1, -1);
    fVarGlobCnoSPD = fPtVarGlobCnoSPD->ProfileX("_pfxGlobCNoSPD", 1, -1);
    break;
  }

  TH1  *fVarGlobAll(nullptr);
  if (histsGlobAll)
  {
    fVarGlobAll = fPtVarGlobAll->ProjectionY("fVarGlobAll", binMin, fPtPhiGlobAll->GetNbinsX());
    fVarGlobAll->SetLineColor(4);
    fVarGlobAll->SetLineWidth(3);
  }
  fVarGlobSt->SetLineColor(2);
  fVarGlobSt->SetLineWidth(3);

  fVarGlobCnoSPD->SetLineStyle(1);
  fVarGlobCnoSPD->SetLineColor(6);
  fVarGlobCnoSPD->SetLineWidth(3);

  TH1 *fVarGlobCnoITS(nullptr);
  if (histsGlobCnoITS)
  {
    Printf("plot noITSrefit phi dist");

    switch (whichvar)
    {

    case kPhivspt:
    case kEtavspt:
    case kNbClsTPCvspT:
    case kNbCroRowTPCvspT:
      fVarGlobCnoITS = fPtVarGlobCnoITS->ProjectionY("fVarGlobCnoITS", binMin, fPtVarGlobSt->GetNbinsX());
      fVarGlobCnoITS->SetLineStyle(1);
      fVarGlobCnoITS->SetLineColor(8);
      fVarGlobCnoITS->SetLineWidth(3);
      break;

    case kNbClsTPCvsphi:
    case kNbCroRowTPCvsphi:
      TProfile *fVarGlobCnoITS = fPtVarGlobCnoITS->ProfileX("_pfxfVarGlobCnoITS", 1, -1);
      fVarGlobCnoITS->SetLineStyle(1);
      fVarGlobCnoITS->SetLineColor(8);
      fVarGlobCnoITS->SetLineWidth(3);
      break;
    }
  }

  auto fPhiGlobSt = fPtPhiGlobSt->ProjectionY("fPhiGlobSt", binMin, fPtPhiGlobSt->GetNbinsX());
  auto fVarGlobSum = static_cast<TH1 *>(fPhiGlobSt->Clone());
  fVarGlobSum->SetTitle("fPhiGlobSum");
  fVarGlobSum->SetName("fPhiGlobSum");
  fVarGlobSum->Add(fVarGlobCnoSPD);

  gPad->SetLeftMargin(0.2);
  gPad->SetRightMargin(0.02);
  gPad->SetBottomMargin(0.12);

  TH1F *frame2;

  switch (whichvar)
  {

  case kPhivspt:
    frame2 = gPad->DrawFrame(0., 0., 2. * TMath::Pi(), fVarGlobSum->GetBinContent(fVarGlobSum->GetMaximumBin()) * 2.); //1.5);
    frame2->SetXTitle("#phi");
    frame2->SetYTitle("#frac{1}{N_{evts}}#frac{dN}{d#phi}");
    break;

  case kEtavspt:
    frame2 = gPad->DrawFrame(-1.5, 0., 1.5, fVarGlobSum->GetBinContent(fVarGlobSum->GetMaximumBin()) * 2.);
    frame2->SetXTitle("#eta");
    frame2->SetYTitle("#frac{1}{N_{evts}}#frac{dN}{d#eta}");
    break;

  case kNbClsTPCvspT:
    frame2 = gPad->DrawFrame(30, 0., 180, fVarGlobSum->GetBinContent(fVarGlobSum->GetMaximumBin()) * 2.);
    frame2->SetXTitle("Nb Cls TPC");
    frame2->SetYTitle("#frac{1}{N_{evts}}#frac{dN}{d NbClsTPC}");
    break;

  case kNbClsTPCvsphi:
    frame2 = gPad->DrawFrame(30, 0., 180, fVarGlobSum->GetBinContent(fVarGlobSum->GetMaximumBin()) * 2.);
    frame2->SetXTitle("#phi");
    frame2->SetYTitle("#frac{1}{N_{evts}}#frac{dN} <NbClsRowsTPC>");
    break;

  case kNbCroRowTPCvspT:
    frame2 = gPad->DrawFrame(30, 0., 180, fVarGlobSum->GetBinContent(fVarGlobSum->GetMaximumBin()) * 2.);
    frame2->SetXTitle("Nb Crowssed Rows TPC");
    frame2->SetYTitle("#frac{1}{N_{evts}}#frac{dN}{d NbCrsRowsTPC}");
    break;

  case kNbCroRowTPCvsphi:
    frame2 = gPad->DrawFrame(30, 0., 180, fVarGlobSum->GetBinContent(fVarGlobSum->GetMaximumBin()) * 2.);
    frame2->SetXTitle("#phi");
    frame2->SetYTitle("#frac{1}{N_{evts}}#frac{dN} <NbCrsRowsTPC>");
    break;
  }

  frame2->GetXaxis()->SetTitleSize(0.06);
  frame2->GetYaxis()->SetTitleSize(0.06);
  frame2->GetXaxis()->SetTitleOffset(0.75);
  frame2->GetYaxis()->SetTitleOffset(1.4);

  // fPhiGlobAll->DrawCopy("same");

  fVarGlobSt->DrawCopy("same");
  fVarGlobCnoSPD->DrawCopy("same");

  TH1 *fVarGlobSum2(nullptr);
  if (histsGlobCnoITS)
  {
    auto fPhiGlobCnoITS = fPtPhiGlobCnoITS->ProjectionY("fPhiGlobCnoITS", binMin, fPtPhiGlobSt->GetNbinsX());
    fVarGlobCnoITS->DrawCopy("same");
    fVarGlobSum2 = static_cast<TH1 *>(fVarGlobSt->Clone());
    fVarGlobSum2->SetTitle("fPhiGlobSum2");
    fVarGlobSum2->SetName("fPhiGlobSum2");
    fVarGlobSum2->Add(fPhiGlobCnoITS);
    fVarGlobSum2->SetLineColor(4);
    fVarGlobSum2->SetMarkerColor(4);
    fVarGlobSum2->DrawCopy("same");
  }

  fVarGlobSum->SetLineColor(1);
  fVarGlobSum->SetMarkerColor(1);
  fVarGlobSum->DrawCopy("same");

  auto leg2 = new TLegend(0.22, 0.6, 0.88, 0.88, Form("Hybrid tracks. run:%d", run));
  leg2->SetFillColor(10);
  leg2->SetBorderSize(0);
  leg2->SetFillStyle(0);
  leg2->SetTextSize(0.06);
  leg2->AddEntry(fVarGlobSt, "w/ SPD & ITSrefit", "l");
  leg2->AddEntry(fVarGlobCnoSPD, "w/o SPD & w/ ITSrefit", "l");
  leg2->AddEntry(fVarGlobSum, "sum", "l");
  if (histsGlobCnoITS)
  {
    leg2->AddEntry(fVarGlobCnoITS, "w/o SPD || w/o ITSrefit", "l");
    leg2->AddEntry(fVarGlobSum2, "Incl w/o ITSrefit", "l");
  }
  leg2->Draw();

  //NEventsGlobSt
  TLatex textNEvents;
  textNEvents.SetNDC();
  textNEvents.DrawLatex(0.52, 0.42, Form("#it{N}_{events} = %.0f", NEventsGlobSt));

  switch (whichvar)
  {

  case kPhivspt:
    c2->SaveAs(Form("PhiCent%d%sRun%d.png", cent, strTrigger.Data(), run));
    c2->SaveAs(Form("PhiCent%d%sRun%d.eps", cent, strTrigger.Data(), run));
    break;

  case kEtavspt:
    c2->SaveAs(Form("EtaCent%d%sRun%d.png", cent, strTrigger.Data(), run));
    c2->SaveAs(Form("EtaCent%d%sRun%d.eps", cent, strTrigger.Data(), run));
    break;

  case kNbClsTPCvspT:
    c2->SaveAs(Form("ClsTPCCent%d%sRun%d.png", cent, strTrigger.Data(), run));
    c2->SaveAs(Form("ClsTPCCent%d%sRun%d.eps", cent, strTrigger.Data(), run));
    break;

  case kNbClsTPCvsphi:
    c2->SaveAs(Form("ClsTPCPhiCent%d%sRun%d.png", cent, strTrigger.Data(), run));
    c2->SaveAs(Form("ClsTPCPhiCent%d%sRun%d.eps", cent, strTrigger.Data(), run));
    break;

  case kNbCroRowTPCvspT:
    c2->SaveAs(Form("RowsTPCCent%d%sRun%d.png", cent, strTrigger.Data(), run));
    c2->SaveAs(Form("RowsTPCCent%d%sRun%d.eps", cent, strTrigger.Data(), run));
    break;

  case kNbCroRowTPCvsphi:
    c2->SaveAs(Form("RowsTPCPPhiCent%d%sRun%d.png", cent, strTrigger.Data(), run));
    c2->SaveAs(Form("RowsTPCPhiCent%d%sRun%d.eps", cent, strTrigger.Data(), run));
    break;
  }

  auto c3 = new TCanvas("c3", "c3: Pt spectra", 600, 450);

  gPad->SetLeftMargin(0.2);
  gPad->SetRightMargin(0.02);
  gPad->SetBottomMargin(0.12);

  TH1F *frame3 = gPad->DrawFrame(0., 1e-7, 100., 1e5);
  frame3->SetXTitle("p_{T} [GeV]");
  frame3->SetYTitle("#frac{1}{N_{evts}}#frac{dN}{dp_{T}} [GeV^{-1}]");
  frame3->GetXaxis()->SetTitleSize(0.06);
  frame3->GetYaxis()->SetTitleSize(0.06);
  frame3->GetXaxis()->SetTitleOffset(0.75);
  frame3->GetYaxis()->SetTitleOffset(1.4);

  gPad->SetLogy();

  TH1 *fPtGlobSt = fPtVarGlobSt->ProjectionX("fPtGlobSt"),
      *fPtGlobCnoSPD = fPtVarGlobCnoSPD->ProjectionX("fPtGlobCnoSPD"),
      *fPtGlobAll(nullptr);
  if (histsGlobAll)
    fPtGlobAll = fPtVarGlobAll->ProjectionX("fPtGlobAll");

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

  if (kPhivspt)
  {
    c3->SaveAs(Form("PtCent%d.png", cent));

    std::unique_ptr<TFile>histos(TFile::Open("TrackPtSpectra.root", "RECREATE"));

    fPtGlobSt->Write("fPtGlobSt");
    fPtGlobCnoSPD->Write("fPtGlobCnoSPD");
  }
}
