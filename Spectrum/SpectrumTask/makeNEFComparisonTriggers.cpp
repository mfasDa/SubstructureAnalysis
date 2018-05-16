#ifndef __CLING__
#include <array>
#include <functional>
#include <iomanip>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <ROOT/TSeq.hxx>
#include "RStringView.h"
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#include <TPaveText.h>
#endif

#include "../../helpers/graphics.C"

struct RangedSpectrum {
  double fPtMin;
  double fPtMax;
  TH1 *fData;

  bool operator==(const RangedSpectrum &other) const { return fPtMin == other.fPtMin && fPtMax == other.fPtMax; }
  bool operator<(const RangedSpectrum &other) const { return fPtMax < other.fPtMax; }
};

TH1 *makeNEFProjection(THnSparse *hsparse, double ptmin, double ptmax) {
  const double kVerySmall = 1e-5;
  int binptmin = hsparse->GetAxis(0)->FindBin(ptmin+kVerySmall), binptmax = hsparse->GetAxis(0)->FindBin(ptmax - kVerySmall);
  hsparse->GetAxis(0)->SetRange(binptmin, binptmax);

  auto projected = hsparse->Projection(3);
  projected->SetDirectory(nullptr);
  hsparse->GetAxis(0)->UnZoom();

  return projected;
}

std::vector<RangedSpectrum> getNEFSpectraForTrigger(TFile &reader, double radius,  const std::string_view trigger, const std::string_view triggercluster){
  std::unordered_map<std::string, int> triggerclustermapping = {
    {"ANY", 0}, {"CENT", 1}, {"CENTNOTRD", 2}, {"CALO", 3}, {"CALOFAST", 4}, {"CENTBOTH", 5}, {"ONLYCENT", 6}, {"ONLYCENTNOTRD", 7},
    {"CALOBOTH", 8}, {"ONLYCALO", 9}, {"ONLYCALOFAST", 10}
  };
  std::array<std::pair<double, double>, 9> ptbins = {{{20., 40.}, {40., 60.}, {60., 80.}, {80., 100.}, {100., 120.}, {120., 140.}, {140., 160.}, {160., 180.}, {180., 200.}}};
  std::vector<RangedSpectrum> result;
  std::stringstream dirname;
  dirname << "JetSpectrum_FullJets_R" << std::setw(2) << std::setfill('0') << int(radius * 10.) << "_" << trigger;
  reader.cd(dirname.str().data());
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto hsparse = static_cast<THnSparse *>(histlist->FindObject("hJetTHnSparse"));
  hsparse->GetAxis(3)->SetRangeUser(0., 0.98);
  auto clusterbin = hsparse->GetAxis(4)->FindBin(triggerclustermapping.find(std::string(triggercluster))->second);
  hsparse->GetAxis(4)->SetRange(clusterbin, clusterbin);

  for(auto pt : ptbins) {
    auto hist = makeNEFProjection(hsparse, pt.first, pt.second);
    hist->SetDirectory(nullptr);
    std::stringstream histname;
    histname << "R" << std::setw(2) << std::setfill('0') << int(radius*10.) << "_" << trigger << "_" << int(pt.first) << "_" << int(pt.second);
    hist->SetName(histname.str().data());
    hist->Scale(1./hist->Integral());
    result.emplace_back(RangedSpectrum{double(pt.first), double(pt.second), hist});
  }

  hsparse->GetAxis(3)->UnZoom();
  hsparse->GetAxis(4)->UnZoom();

  std::sort(result.begin(), result.end(), std::less<RangedSpectrum>());
  return result;
}

void makeNEFComparisonTriggers(double r, const std::string_view triggercluster){
  auto plot = new TCanvas(Form("NEFcomparisonR%02d", int(r*10.)), Form("NEF Comparison for R=%0.1f", r), 1200, 1100);
  plot->Divide(3,3);

  auto leg = new TLegend(0.65, 0.45, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  std::array<TPaveText *, 9> labels;
  memset(labels.data(), 0, sizeof(TPaveText *) * labels.size());

  for(auto i : ROOT::TSeqI(0, 9)){
    plot->cd(i+1);
    auto frame = new TH1F(Form("nefframe%d", i), "; NEF; 1/N_{jet} dN_{jet}/dNEF", 100, 0., 1.);
    frame->SetDirectory(nullptr);
    frame->SetStats(false);
    frame->GetYaxis()->SetRangeUser(0., 0.06);
    frame->Draw("axis");
    
    if(i == 0) leg->Draw();
  }

  std::unique_ptr<TFile> reader(TFile::Open("AnalysisResults.root", "READ"));
  std::map<const std::string, Style> kTriggers = {{"INT7", {kBlack, 20}}, {"EG1", {kRed, 24}}, {"EG2", {kMagenta, 25}}, {"EJ1", {kBlue, 27}}, {"EJ2", {kGreen, 28}}};
  for(auto t : kTriggers) {
    auto hists = getNEFSpectraForTrigger(*reader, r, t.first, triggercluster);
    for(auto ipt : ROOT::TSeqI(0, hists.size())){
      auto pthist = hists[ipt].fData;
      t.second.SetStyle<TH1>(*pthist);
      plot->cd(ipt+1);
      pthist->Draw("epsame");
      if(ipt == 0) leg->AddEntry(pthist, t.first.data(), "lep");
      if(!labels[ipt]) {
        labels[ipt] = new TPaveText(0.15, 0.9, 0.75, 0.99, "NDC");
        InitWidget<TPaveText>(*labels[ipt]);
        labels[ipt]->AddText(Form("%.1f GeV/c < p_{t,jet} < %.1f GeV/c", hists[ipt].fPtMin, hists[ipt].fPtMax));
        labels[ipt]->Draw("axis");
      }
    }
  }

  plot->cd();
  plot->Update();
}