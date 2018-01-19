#ifndef __CLING__
#include <memory>

#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#endif

struct mystyle {
  Color_t col;
  Style_t mark;
};

template<typename t>
std::unique_ptr<t> make_unique(t *ptr) { 
  return std::unique_ptr<t>(ptr); 
}

TH1 *Read(std::string_view inputfile){
  auto reader = make_unique<TFile>(TFile::Open(inputfile.data(), "READ"));
  auto result = static_cast<TH1 *>(reader->Get("efficiency"));
  result->SetDirectory(nullptr);
  return result;
}

TH1 * Style(TH1 *input, std::string_view title, mystyle style) {
  input->SetTitle(title.data());
  input->SetMarkerColor(style.col);
  input->SetMarkerStyle(style.mark);
  input->SetLineColor(style.col);
  return input;
}

void makeComparisonTrackingEff(){
  auto eff7 = Style(Read("7TeV/TrackingEfficiency.root"), "pp, #sqrt{s} = 7 TeV", {kRed, 20}),
       eff276 =  Style(Read("276TeV/146805/TrackingEfficiency.root"), "pp, #sqrt{s} = 2.76 TeV", {kGreen, 21}),
       eff13 =  Style(Read("13TeV/merged/TrackingEfficiency.root"), "pp, #sqrt{s} = 13 TeV", {kBlue, 22});
  
  auto plot = new TCanvas("trackingplot", "Tracking eff", 800, 600);
  plot->cd();

  auto axis = new TH1F("axis", "; p_{t, gen} (GeV/c); efficiency", 100, 0., 100.);
  axis->SetDirectory(nullptr);
  axis->SetStats(false);
  axis->GetYaxis()->SetRangeUser(0., 1.);
  axis->Draw("axis");

  auto leg = new TLegend(0.65, 0.15, 0.89, 0.35);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->SetTextFont(42);
  leg->Draw();

  eff7->Draw("psame");
  eff276->Draw("psame");
  eff13->Draw("psame");
  leg->AddEntry(eff7, eff7->GetTitle(), "p");
  leg->AddEntry(eff276, eff276->GetTitle(), "p");
  leg->AddEntry(eff13, eff13->GetTitle(), "p");

  plot->Update();
}