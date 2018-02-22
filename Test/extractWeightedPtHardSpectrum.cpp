#ifndef __CLING__
#include <memory>
#include <RStringView.h>
#include <ROOT/TSeq.hxx>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#endif

std::pair<double, TH1 *> GetWeightedPtHardSpectrum(std::string_view filename, std::string_view dirname, int bin, bool withscale){
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  reader->cd(dirname.data());
  auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();

  auto crosssection = static_cast<TH1 *>(histlist->FindObject("fHistXsection")),
       trials = static_cast<TH1 *>(histlist->FindObject("fHistTrials"));
  /* 
  auto bin = -1;
  for(auto b : ROOT::TSeqI(1, trials->GetXaxis()->GetNbins() + 1)) {
    if(trials->GetBinContent(b)) {
      bin = b;
      break;
    }
  }
  */
  auto weight = crosssection->GetBinContent(bin) / trials->GetBinContent(bin);
  auto ptspec = static_cast<TH1 *>(histlist->FindObject("fHistPtHard"));
  ptspec->SetDirectory(nullptr);
  if(withscale) ptspec->Scale(weight);
  return std::pair<double, TH1 *>(weight, ptspec);
}

void extractWeightedPtHardSpectrum(std::string_view dirname, bool withscale){
  TH1 *merged = nullptr, *weights = new TH1F("weights", "weights", 20, 0., 20.);
  for(auto b : ROOT::TSeqI(1, 21)){
    auto scaled = GetWeightedPtHardSpectrum(Form("%02d/AnalysisResults.root", b), dirname, b+1, withscale);
    weights->SetBinContent(b+1, scaled.first);
    if(!merged) merged = scaled.second;
    else {
      merged->Add(scaled.second);
      delete scaled.second;
    }
  }

  auto plot = new TCanvas("scaled", "scaled", 800, 600);
  plot->SetLogy();
  merged->Draw();

  auto weightplot = new TCanvas("weightplot", "weightplot", 800, 600);
  weightplot->SetLogy();
  weights->Draw();
}