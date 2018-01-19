#ifndef __CLING__
#include <memory>
#include <tuple>
#include <vector>

#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TPaveText.h>
#endif

struct mystyle {
  Color_t color;
  Style_t marker;

  void Apply(TH1 *tobestyled) {
    tobestyled->SetMarkerColor(color);
    tobestyled->SetMarkerStyle(marker);
    tobestyled->SetLineColor(color);
  }  
};

TH1 *ReadJES(std::string_view filename, std::string_view legentry, mystyle plotstyle) {
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  TH1 *jes = static_cast<TH1 *>(reader->Get("ptdiffmean"));
  jes->SetDirectory(nullptr);
  jes->SetTitle(legentry.data());
  plotstyle.Apply(jes);
  return jes;
}

void makeComparisonJES13TeVESD() {
  std::vector<std::tuple<std::string, std::string, mystyle>> datasets = {
    std::make_tuple<std::string, std::string, mystyle>("1425_jes_wnorefit/merged/EnergyScale_R02.root", "R=0.2, with non-refit tracks", {kRed, 24}),
    std::make_tuple<std::string, std::string, mystyle>("1425_jes_wnorefit/merged/EnergyScale_R04.root", "R=0.4, with non-refit tracks", {kRed, 25}),
    std::make_tuple<std::string, std::string, mystyle>("1426_jes_wonorefit/merged/EnergyScale_R02.root", "R=0.2, without non-refit tracks", {kBlue, 24}),
    std::make_tuple<std::string, std::string, mystyle>("1426_jes_wonorefit/merged/EnergyScale_R04.root", "R=0.4, without non-refit tracks", {kBlue, 25})
  };   

  auto plot = new TCanvas("ComparisonPlot", "Comparison plot", 800, 600);
  plot->cd();

  auto axis = new TH1F("axis", "; p_{t, part} (GeV); (p_{t, det} = p_{t, part})/p_{t, part}", 200, 0., 200.);
  axis->SetStats(false);
  axis->GetYaxis()->SetRangeUser(-1., 1.);
  axis->Draw("axis");

  auto label = new TPaveText(0.15, 0.15, 0.75, 0.22, "NDC");
  label->SetBorderSize(0);
  label->SetFillStyle(0);
  label->SetTextFont(42);
  label->AddText("pp #sqrt{s} = 13 TeV, LHC17f8a, ESDs, hybrid tracks (2011)");
  label->Draw();

  auto legend = new TLegend(0.5, 0.7, 0.89, 0.89);
  legend->SetBorderSize(0);
  legend->SetFillStyle(0);
  legend->SetTextFont(42);
  legend->Draw();

  for(auto data : datasets) {
    auto hist = ReadJES(std::get<0>(data), std::get<1>(data), std::get<2>(data));
    hist->Draw("epsame");
    legend->AddEntry(hist, hist->GetTitle(), "lep");
  }
  plot->Update();
}