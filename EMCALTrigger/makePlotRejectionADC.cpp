#ifndef __CLING__
#include <memory>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#include <TMath.h>
#include <TPaveText.h>
#include <TString.h>
#endif

#include "../helpers/graphics.C"

struct RejectionData{
  TH1       *fHistMinBias, *fHistLow, *fHistHigh;

  int GetThreshold(bool high) const {
    auto maxbinhandler =  [](TH1 *hist) -> int { return hist->GetMaximumBin(); };
    return maxbinhandler(high ? fHistHigh : fHistLow) - 1;
  }

  double GetRejection(bool high) const {
    return fHistMinBias->Integral() / fHistMinBias->Integral(GetThreshold(high) + 1, fHistMinBias->GetXaxis()->GetNbins());
  }
};

RejectionData getRejectionData(TFile &reader, bool emcal, bool jettrigger) {
  TString histtemplate = Form("hPatchADCMax%s%sRecalc", emcal ? "E" : "D", jettrigger ? "JE" : "GA");
  reader.cd("MaxPatchINT7");
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto mbhist = static_cast<TH1 *>(histlist->FindObject(histtemplate.Data()));
  mbhist->SetDirectory(nullptr);
  mbhist->SetName("INT7");
  mbhist->SetTitle("INT7");
  
  reader.cd(Form("MaxPatch%s%s2", emcal ? "E" : "D", jettrigger ? "J" : "G"));
  histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto lowhist = static_cast<TH1 *>(histlist->FindObject(histtemplate.Data()));
  lowhist->SetDirectory(nullptr);
  lowhist->SetName(Form("%s%s2", emcal ? "E" : "D", jettrigger ? "J" : "G"));
  lowhist->SetTitle(Form("%s%s2", emcal ? "E" : "D", jettrigger ? "J" : "G"));

  reader.cd(Form("MaxPatch%s%s1", emcal ? "E" : "D", jettrigger ? "J" : "G"));
  histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto highhist = static_cast<TH1 *>(histlist->FindObject(histtemplate.Data()));
  highhist->SetDirectory(nullptr);
  highhist->SetName(Form("%s%s2", emcal ? "E" : "D", jettrigger ? "J" : "G"));
  highhist->SetTitle(Form("%s%s2", emcal ? "E" : "D", jettrigger ? "J" : "G"));

  return {mbhist, lowhist, highhist};
}

int getndigits(int number) {
  int digits = 0;
  while(number / TMath::Power(10, digits) > 0) digits++;
  return digits;
}

void ToPad(const RejectionData &data) {
  gPad->SetLogy();
  auto frame = new TH1F(Form("frame_%s_%s_%s", data.fHistMinBias->GetName(), data.fHistLow->GetName(), data.fHistLow->GetName()), "; Max Patch ADC; Number of events", 1000, 0., 1000.);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(0.5, data.fHistMinBias->GetMaximum());
//+ TMath::Power(10, getndigits(data.fHistMinBias->GetMaximum()) - 1)
  frame->Draw("axis");

  auto mbstyle = Style{kBlack, 20};
  mbstyle.SetStyle<TH1>(*data.fHistMinBias);
  data.fHistMinBias->SetStats(false);
  data.fHistMinBias->Draw("epsame");

  auto lowstyle = Style{kBlue, 24};
  lowstyle.SetStyle<TH1>(*data.fHistLow);
  data.fHistLow->SetStats(false);
  data.fHistLow->Draw("epsame");

  auto highstyle = Style{kRed, 25};
  highstyle.SetStyle<TH1>(*data.fHistHigh);
  data.fHistHigh->SetStats(false);
  data.fHistHigh->Draw("epsame");

  auto leg = new TLegend(0.65, 0.65, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->AddEntry(data.fHistMinBias, data.fHistMinBias->GetTitle(), "lep");
  leg->AddEntry(data.fHistLow, Form("%s: R = %.1f", data.fHistLow->GetTitle(), data.GetRejection(false)), "lep");
  leg->AddEntry(data.fHistHigh, Form("%s: R = %.1f", data.fHistHigh->GetTitle(), data.GetRejection(true)), "lep");
  leg->Draw();
}

void makePlotRejectionADC(const char *filename = "AnalysisResults.root"){
  auto plot = new TCanvas("rejectionADC", "Rejection factors from ADC", 1200, 1000);
  plot->Divide(2,2);

  auto reader = std::unique_ptr<TFile>(TFile::Open(filename, "READ"));
  plot->cd(1);
  ToPad(getRejectionData(*reader, true, false));
  plot->cd(2);
  ToPad(getRejectionData(*reader, true, true));
  plot->cd(3);
  ToPad(getRejectionData(*reader, false, false));
  plot->cd(4);
  ToPad(getRejectionData(*reader, false, true));
}