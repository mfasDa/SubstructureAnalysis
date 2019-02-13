#ifndef __CLING__
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "RStringView.h"
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TList.h>
#include <TSystem.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../helpers/filesystem.C"
#include "../helpers/graphics.C"
#include "../helpers/math.C"
#include "../helpers/string.C"

class PeriodTrending{
public:
  PeriodTrending() = default;
  ~PeriodTrending() = default;

  void AddTrendPoint(const std::string_view &period, double val, double error) { fData.insert({(std::string)period, val, error}); };
  TH1 *CreateTrendingHistogram(const std::string_view name, const std::string_view title) const {
    std::cout << "Found data with " << fData.size() << " periods " << std::endl;
    auto result = new TH1F(name.data(), title.data(), fData.size(), -0.5, fData.size() - 0.5);
    result->SetDirectory(nullptr);
    result->SetStats(false);
    int bincounter(0);
    for(auto d : fData){
      result->GetXaxis()->SetBinLabel(bincounter+1, d.fPeriodName.data());
      result->SetBinContent(bincounter+1, d.fValue);
      result->SetBinError(bincounter+1, d.fError);
      bincounter++;
    }
    return result;
  }
private:
  struct Trendpoint {
    std::string             fPeriodName;
    Double_t                fValue;
    Double_t                fError;

    bool operator==(const Trendpoint &other) const { return fPeriodName == other.fPeriodName; }
    bool operator<(const Trendpoint &other) const { return fPeriodName < other.fPeriodName; }
  };
  
  std::set<Trendpoint>      fData;
};

struct ptrange {
    double fPtMin;
    double fPtMax;

    bool operator==(const ptrange &other) const { return fPtMin == other.fPtMin && fPtMax == other.fPtMax; }
    bool operator<(const ptrange &other) const { return fPtMax <= other.fPtMin; }
};

std::set<std::string> getListOfPeriods(const std::string_view inputdir){
  std::set<std::string> result;
  for(auto d : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data())){
    if(d.find("LHC") == 0)
      result.insert(d);
  }
  return result;
}

TH1 *getNormalizedSpectrum(const std::string_view filename, const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL){
  std::cout << "Reading " << filename << std::endl;
  if(gSystem->AccessPathName(filename.data())) return nullptr;
  try {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto dir = static_cast<TDirectoryFile *>(reader->Get(Form("ChargedParticleQA_%s_nocorr", tracktype.data())));
    auto histlist = static_cast<TList *>(static_cast<TKey *>(dir->GetListOfKeys()->At(0))->ReadObj());
    auto norm = static_cast<TH1 *>(histlist->FindObject(Form("hEventCount%s", trigger.data())));
    auto spec = std::unique_ptr<THnSparse>(static_cast<THnSparse *>(histlist->FindObject(Form("hPtEtaPhiAll%s", trigger.data()))));
    // Look in front of EMCAL
    if(restrictEMCAL){
      spec->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      spec->GetAxis(2)->SetRangeUser(1.4, 3.1);
    }
    auto projected = spec->Projection(0);
    projected->SetDirectory(nullptr);
    projected->Scale(1./norm->GetBinContent(1));
    normalizeBinWidth(projected);
    return projected;
  } catch (...) {
    std::cout << "Failure ... " << std::endl;
    return nullptr;
  }
}

double getmin(const TGraphErrors *g){
  auto result = DBL_MAX;
  for(auto p : ROOT::TSeqI(g->GetN())) {
    auto test = g->GetY()[p] - g->GetEY()[p];
    if(test < result) result = test;
  }
  return result;
}

double getmax(const TGraphErrors *g) {
  auto result = DBL_MIN;
  for(auto p : ROOT::TSeqI(g->GetN())) {
    auto test = g->GetY()[p] + g->GetEY()[p];
    if(test > result) result = test;
  }
  return result;
}

ROOT6tools::TSavableCanvas *MakePlot(int index, const std::vector<std::string> &listofruns, const std::string_view inputdir, const std::string_view tracktype, const std::string_view trigger, const std::string_view acceptance, std::map<ptrange, PeriodTrending> &trendgraphs){
  auto plot = new ROOT6tools::TSavableCanvas(Form("TrackComparison_%s_%s_%s_%d", tracktype.data(), trigger.data(), acceptance.data(), index), Form("Track comparison %s track (%s, %s) %d", tracktype.data(), trigger.data(), acceptance.data(), index), 800, 600);
  plot->cd();
  plot->SetLogy();

  (new ROOT6tools::TAxisFrame(Form("spectrumframe%s%s%d", tracktype.data(), trigger.data(), index), "p_{t} (GeV/c)", "1/N_{trg} dN_{trk}/dp_{t} ((GeV/c)^{-1}", 0., 100., 1e-9, 100.))->Draw("axis");
  auto trklab =new ROOT6tools::TNDCLabel(0.15, 0.84, 0.5, 0.89, Form("Track type: %s", tracktype.data()));
  trklab->SetTextAlign(12);
  trklab->Draw();
  auto trglab = new ROOT6tools::TNDCLabel(0.15, 0.78, 0.4, 0.83, Form("Trigger: %s", trigger.data()));
  trglab->SetTextAlign(12);
  trglab->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.75, 0.45, 0.89, 0.89);
  leg->Draw();
  
  const std::array<Color_t, 10> colors = {{kRed,kBlue, kGreen, kViolet, kOrange, kMagenta, kTeal, kGray, kAzure, kBlack}};
  const std::array<Style_t, 7> markers = {{24, 25, 26, 27, 28, 29, 30}};
  int ispec = 0, icol = 0, imrk = 0;
  for(auto p : listofruns){
    auto spec = getNormalizedSpectrum(Form("%s/%s/AnalysisResults.root", inputdir.data(), p.data()), tracktype, trigger, acceptance == "EMCAL");
    if(!spec){
      for(auto &t : trendgraphs){
        t.second.AddTrendPoint(p, 0., 0.);
      }
      continue;
    }
    spec->SetName(Form("%s_%s_%s", trigger.data(), tracktype.data(), p.data()));
    Style{colors[icol++], markers[imrk++]}.SetStyle<TH1>(*spec);
    if(icol == 10) icol = 0;
    if(imrk == 7) imrk = 0;
    spec->Draw("epsame");
    leg->AddEntry(spec, p.data(), "lep");

    // fill trending graphs
    for(auto &t : trendgraphs){
      auto binptmin = spec->GetXaxis()->FindBin(t.first.fPtMin + 1e-5),
           binptmax = spec->GetXaxis()->FindBin(t.first.fPtMax - 1e-5);
      double ig, err;
      ig = spec->IntegralAndError(binptmin, binptmax, err);
      t.second.AddTrendPoint(p, ig, err);
    }
  }
  plot->Update();
  return plot;
}
void compareTracksPeriodByPeriodV1(const std::string_view track_type, const std::string_view trigger, const std::string_view acceptance, const std::string_view inputdir){
  std::map<ptrange, PeriodTrending> trending = {{{0.5, 0.6}, PeriodTrending()}, {{1., 1.2}, PeriodTrending()}, {{2., 2.4}, PeriodTrending()}, 
                                               {{5., 6.}, PeriodTrending()}, {{10., 14.}, PeriodTrending()}, {{20., 30.}, PeriodTrending()}};
  auto periods = getListOfPeriods(inputdir);
  if(!periods.size()){
    std::cout << "No periods found in input dir " << inputdir << ", skipping ..." << std::endl;
    return; 
  } else {
    std::cout << "Found " << periods.size() << " periods in input dir " << inputdir << " ..." << std::endl;
  }

  int canvas = 0;
  std::vector<std::string> periodsplot;
  for(auto p : periods) {
    periodsplot.emplace_back(p);
    if(periods.size() == 10) {
      auto compplot = MakePlot(canvas++, periodsplot, inputdir, track_type, trigger, acceptance, trending);
      compplot->SaveCanvas(compplot->GetName());
      periodsplot.clear();
    }
  }
  if(periodsplot.size()){
    auto compplot = MakePlot(canvas++, periodsplot, inputdir, track_type, trigger, acceptance, trending);
    compplot->SaveCanvas(compplot->GetName());
  }

  // Draw trending
  auto trendplot = new ROOT6tools::TSavableCanvas(Form("trackTrendingRange_%s_%s_%s", track_type.data(), trigger.data(), acceptance.data()), Form("Track trending %s track %s %s", track_type.data(), trigger.data(), acceptance.data()), 1200, 1000);
  trendplot->Divide(3,2);
  Style trendstyle{kBlack, 20};

  std::set<ptrange> ptref;
  int ipad = 1;
  std::unique_ptr<TFile> writer(TFile::Open(Form("trendingRange_%s_%s_%s.root", track_type.data(), trigger.data(), acceptance.data()), "RECREATE"));
  writer->cd();
  for(auto en : trending) ptref.insert(en.first);   // no c++17, so sad ...
  for(auto pt : ptref ){
    trendplot->cd(ipad++);
    gPad->SetLeftMargin(0.18);
    gPad->SetRightMargin(0.1);
    auto trendhist = trending.find(pt)->second.CreateTrendingHistogram(Form("trending_%02d_%02d_%s_%s", int(pt.fPtMin * 10.), int(pt.fPtMax * 10.), track_type.data(), trigger.data()), Form("Track trending %s tracks %.1f GeV/c < p_{t} < %.1f GeV/c %s", track_type.data(), pt.fPtMin, pt.fPtMax, trigger.data()));
    trendhist->GetYaxis()->SetTitle(Form("1/N_{trg} dN/dp_{t}|_{p_{t} = 5 GeV/c} ((GeV/c)^{-1}"));
    trendstyle.SetStyle<TH1>(*trendhist);
    trendhist->Draw("pe");
    if(ipad == 2){
      auto trklab =new ROOT6tools::TNDCLabel(0.2, 0.84, 0.55, 0.89, Form("Track type: %s", track_type.data()));
      trklab->SetTextAlign(12);
      trklab->Draw();
      auto trglab = new ROOT6tools::TNDCLabel(0.2, 0.78, 0.45, 0.83, Form("Trigger: %s", trigger.data()));
      trglab->SetTextAlign(12);
      trglab->Draw();
    }
    trendhist->Write(trendhist->GetName());
  }
  trendplot->SaveCanvas(trendplot->GetName());
}