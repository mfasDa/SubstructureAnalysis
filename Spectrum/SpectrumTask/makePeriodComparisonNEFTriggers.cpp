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


class NEFPlot : public TCanvas {
public:
  NEFPlot() = default;
  NEFPlot(const char *name, const char *title, int width, int hight) : TCanvas(name, title, width, hight), fLegend(nullptr), fLabels() { memset(fLabels.data(), 0, sizeof(TPaveText *) * fLabels.size()); }
  ~NEFPlot() override = default;

  void SetLegend(TLegend *leg) { fLegend = leg; }
  TLegend *GetLegend() const { return fLegend; }

  void AddLabelAt(TPaveText *label, int ipad) { fLabels[ipad] = label; }
  bool HasPadLabel(int ipad) const { return fLabels[ipad]; }

private:
  TLegend                           *fLegend;
  std::array<TPaveText *, 9>        fLabels;
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

std::vector<RangedSpectrum> getNEFSpectraForTrigger(TFile &reader, double radius,  const std::string_view trigger, const std::string_view period, const std::string_view triggercluster){
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
    histname << "R" << std::setw(2) << std::setfill('0') << int(radius*10.) << "_" << trigger << "_" << period << "_" << int(pt.first) << "_" << int(pt.second);
    hist->SetName(histname.str().data());
    hist->Scale(1./hist->Integral());
    result.emplace_back(RangedSpectrum{double(pt.first), double(pt.second), hist});
  }

  hsparse->GetAxis(3)->UnZoom();
  hsparse->GetAxis(4)->UnZoom();

  std::sort(result.begin(), result.end(), std::less<RangedSpectrum>());
  return result;
}

NEFPlot *prepareCanvasForTrigger(const std::string_view trigger, double r) {
  auto plot = new NEFPlot(Form("NEFcomparison%sR%02d", trigger.data(), int(r*10.)), Form("NEF Comparison for R=%0.1f in trigger %s", r, trigger.data()), 1200, 1100);
  plot->Divide(3,3);

  auto leg = new TLegend(0.65, 0.45, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  plot->SetLegend(leg);

  for(auto i : ROOT::TSeqI(0, 9)){
    plot->cd(i+1);
    auto frame = new TH1F(Form("nefframe%dR%02d%s", i, int(r*10.), trigger.data()), "; NEF; 1/N_{jet} dN_{jet}/dNEF", 100, 0., 1.);
    frame->SetDirectory(nullptr);
    frame->SetStats(false);
    frame->GetYaxis()->SetRangeUser(0., 0.06);
    frame->Draw("axis");
    
    if(i == 0) leg->Draw();
  }
  return plot;
}

void makePeriodComparisonNEFTriggers(double r, const std::string_view triggercluster){
  const std::map<const std::string , Style> kPeriods = {{"LHC16i", {kBlue, 25}}, {"LHC16j", {kGreen, 26}}, {"LHC16k", {kViolet, 27}}};
  const std::array<const std::string, 4> kTriggers = {{"EG1", "EG2", "EJ1", "EJ2"}};

  std::map<const std::string, NEFPlot *> plots;
  for(const auto &trg : kTriggers) plots[trg] = prepareCanvasForTrigger(trg, r);

  for(const auto &p : kPeriods) {
    std::unique_ptr<TFile> reader(TFile::Open(Form("%s/AnalysisResults.root", p.first.data()), "READ"));

    for(const auto &trg : kTriggers) {
      auto histos = getNEFSpectraForTrigger(*reader, r, trg, p.first, triggercluster);
      for(auto ipt : ROOT::TSeqI(0, histos.size())){
        auto myplot = plots[trg];
        myplot->cd(ipt+1);
        p.second.SetStyle<TH1>(*histos[ipt].fData);
        histos[ipt].fData->Draw("epsame");
        if(ipt == 0) myplot->GetLegend()->AddEntry(histos[ipt].fData, p.first.data(), "lep");
        if(!myplot->HasPadLabel(ipt)) {
          auto label = new TPaveText(0.15, 0.9, 0.75, 0.99, "NDC");
          InitWidget<TPaveText>(*label);
          label->AddText(Form("%.1f GeV/c < p_{t,jet} < %.1f GeV/c", histos[ipt].fPtMin, histos[ipt].fPtMax));
          label->Draw("axis");
          myplot->AddLabelAt(label, ipt);
        }
      }
    }
  }

  for(auto p : plots){
    p.second->cd();
    p.second->Update();
  }
}