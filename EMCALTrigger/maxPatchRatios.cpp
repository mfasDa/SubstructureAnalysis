#ifndef __CLING__
#include <memory>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TList.h>
#include <TLegend.h>
#endif

#include "../helpers/graphics.C"

TH1 *readSpectrum(TFile &reader, const char *trigger, bool jet) {
  reader.cd(Form("MaxPatch%s", trigger));
  auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
  auto hist = static_cast<TH1 *>(histlist->FindObject(Form("hPatchADCMaxE%sRecalc", jet ? "JE" : "GA")));
  hist->SetDirectory(nullptr);
  hist->SetName(trigger);
  return hist;
}

TCanvas *plotMaxPatchRatios(TFile &reader, bool jet) {
  auto mbref = readSpectrum(reader, "INT7", jet),
       low = readSpectrum(reader, jet ? "EJ2" : "EG2", jet),
       high = readSpectrum(reader, jet ? "EJ1" : "EG1", jet);
  high->Divide(low);
  high->SetTitle(Form("%s / %s" , high->GetName(), low->GetName()));
  low->Divide(mbref);
  low->SetTitle(Form("%s / %s" , low->GetName(), mbref->GetName()));
  delete mbref;

  Style{kBlack, 20}.SetStyle<TH1>(*high);
  Style{kRed, 21}.SetStyle<TH1>(*low);

  auto plot = new TCanvas(Form("ratio%s", jet ? "JE" : "GA"), Form("Max patch ratio %s" , jet ? "jet" : "gamma"), 800, 600);
  auto frame = new TH1F(Form("frame%s", jet ? "JE" : "GA"), "; ADC; Ratio spectra", 1000, 0., 1000);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(0., 150.);
  frame->Draw("axis");

  auto leg = new TLegend(0.65, 0.7, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->Draw();

  high->Draw("epsame");
  leg->AddEntry(high, high->GetTitle(), "lep");
  low->Draw("epsame");
  leg->AddEntry(low, low->GetTitle(), "lep");

  plot->Update();
  return plot;
}

void maxPatchRatios(const std::string_view filename){
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  plotMaxPatchRatios(*reader, true);
  plotMaxPatchRatios(*reader, false);
}