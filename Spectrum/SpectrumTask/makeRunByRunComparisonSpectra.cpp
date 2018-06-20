#ifndef __CLING__
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <RStringView.h>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TKey.h>
#include <TList.h>
#include <TROOT.h>
#include <TSystem.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"

struct RunSpectra{
  int                           fRunNumber;
  std::map<double, TH1 *>       fJetSpectra;  

  bool operator==(const RunSpectra &other) const { return fRunNumber == other.fRunNumber; }
  bool operator<(const RunSpectra &other) const { return fRunNumber < other.fRunNumber; }
};

struct Trendpoint {
  int fRun;
  double fRadius;
  double fPt;
  double fVal;
  double fError;

  bool operator==(const Trendpoint &other) const { return fRun == other.fRun; }
  bool operator<(const Trendpoint &other) const { return fRun < other.fRun; }
};

struct TrendingCollection {
  std::vector<Trendpoint> fData;

  TGraphErrors *makeTrend(double radius, double pt){
    std::set<Trendpoint> points;
    for(const auto &d : fData) {
      if(d.fRadius == radius && d.fPt == pt) points.insert(d);
    }
    TGraphErrors *result = new TGraphErrors;
    int npoint(0);
    for(const auto &p : points){
      result->SetPoint(npoint, p.fRun, p.fVal);
      result->SetPointError(npoint, 0, p.fError);
      npoint++;
    }
    return result;
  }
};

const std::array<double, 4> kJetRadii = {{0.2, 0.3, 0.4, 0.5}};
const std::array<double, 4> kTrendPt = {{20., 40., 60., 100.}};

std::map<double, TH1 *> readJetSpectra(const std::string_view filename, const std::string_view jettype, const std::string_view trigger){
  std::map<double, TH1 *> spectra;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  std::stringstream dirname;
  dirname << jettype << "_" << trigger;
  auto dir = static_cast<TDirectoryFile *>(reader->Get(dirname.str().data()));
  auto histlist = CollectionToSTL(dir->GetListOfKeys());

  for(auto rad : kJetRadii){
    std::string radstring = Form("R%02d", int(rad * 10));
    auto spec = static_cast<TH1 *>(static_cast<TKey *>(*std::find_if(histlist.begin(), histlist.end(), [&radstring](const TObject *o) {
      std::string_view histname(o->GetName());
      return histname.find("JetSpectrum") == 0 && histname.find(radstring) != std::string::npos;
    }))->ReadObj());
    spec->SetDirectory(nullptr);
    spectra.insert({rad, spec});    
  }
  return spectra;
}

ROOT6tools::TSavableCanvas *plotSpectra(int index, std::vector<int> runs, const std::string_view jettype, const std::string_view trigger, const std::string_view basedir, const std::string_view filename, TrendingCollection &trend){
  std::set<RunSpectra> rundata;
  std::mutex specmutex;
  auto workfun = [&](int run) {
    auto spec = readJetSpectra(Form("%s/%09d/%s", basedir.data(), run, filename.data()), jettype, trigger);
    std::unique_lock<std::mutex> speclock;
    rundata.insert({run, spec});
  };

  std::vector<std::thread> readers;
  for(const auto &r : runs) readers.emplace_back(workfun, r);
  for(auto &r : readers) r.join();

  std::array<Style, 10> styles = {{{kRed, 24}, {kBlue, 25}, {kGreen, 26}, {kViolet, 27}, {kOrange, 28}, {kMagenta, 29}, {kTeal, 30}, {kGray, 31}, {kAzure, 32}, {kBlack, 33}}};
  auto plot = new ROOT6tools::TSavableCanvas(Form("spectraComparison_%s_%s_%d", jettype.data(), trigger.data(), index), Form("Spectra comparison %s %s %d", jettype.data(), trigger.data(), index), 1200, 1000);
  plot->Divide(2,2);
  
  for(auto ipad : ROOT::TSeqI(0, 4)){
    plot->cd(ipad+1);
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame(Form("specAxis_%s_%s_%d", jettype.data(), trigger.data(), index), "p_{t,jet} (GeV/c)", "1/N_{trg} dN/dp_{t,jet} ((GeV/c)^{-1}", 0., 200., 1e-7, 100))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.74, 0.25, 0.81, Form("R=%.1f", kJetRadii[ipad])))->Draw();
    TLegend *leg(nullptr);
    if(!ipad){
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.5, 0.89, 0.89);
      leg->Draw();
      (new ROOT6tools::TNDCLabel(0.15, 0.82, 0.45, 0.89, Form("%s, %s", jettype.data(), trigger.data())))->Draw();
    }

    int nruns(0);
    for (auto r : rundata){
      auto spec = r.fJetSpectra[kJetRadii[ipad]];
      styles[nruns++].SetStyle<TH1>(*spec);
      spec->Draw("epsame");
      if (leg) leg->AddEntry(spec, Form("%d", r.fRunNumber), "lep");

      std::unique_ptr<TH1> rebinned(static_cast<TH1 *>(spec->Clone("Rebinned")));
      rebinned->Rebin(5);
      rebinned->Scale(1./5);
      for (auto pt : kTrendPt){
        auto bin = rebinned->GetXaxis()->FindBin(pt);
        auto val = rebinned->GetBinContent(bin), err = rebinned->GetBinError(bin);
        trend.fData.push_back({r.fRunNumber, kJetRadii[ipad], pt, val, err});
      }
    }
  }
  plot->cd();
  plot->Update();
  return plot;
}

std::set<int> getListOfRuns(const std::string_view inputdir, const std::string_view filename){
  std::set<int> result;
  for(auto d : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data())){
    if(is_number(d)){
      if(!gSystem->AccessPathName(Form("%s/%s/%s", inputdir.data(), d.data(), filename.data()))) result.insert(std::stoi(d));
    }
  }
  return result;
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

void makeRunByRunComparisonSpectra(const std::string_view jettype, const std::string_view trigger, const std::string_view basedir = ".", const std::string_view filename = "JetSpectra_ANY.root"){
  ROOT::EnableThreadSafety();

  auto runs = getListOfRuns(basedir, filename);
  if(!runs.size()){
    std::cout << "No runs found in input dir " << basedir << ", skipping ..." << std::endl;
    return; 
  } else {
    std::cout << "Found " << runs.size() << " runs in input dir " << basedir << " ..." << std::endl;
  }

  int canvas = 0;
  std::vector<int> runsplot;
  TrendingCollection trending;
  for(auto r : runs) {
    runsplot.emplace_back(r);
    if(runsplot.size() == 10) {
      auto compplot = plotSpectra(canvas++, runsplot, jettype, trigger, basedir,filename, trending);
      compplot->SaveCanvas(compplot->GetName());
      runsplot.clear();
    }
  }
  if(runsplot.size()){
    auto compplot = plotSpectra(canvas++, runsplot, jettype, trigger, basedir,filename, trending);
    compplot->SaveCanvas(compplot->GetName());
  }

    // Draw trending
  auto trendplot = new ROOT6tools::TSavableCanvas(Form("jetTrending_%s_%s", jettype.data(), trigger.data()), Form("Jet trending %s %s", jettype.data(), trigger.data()), 1200, 1000);
  trendplot->Divide(2,2);
  std::map<double, Style> rstyles = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, {0.4, {kGreen, 26}}, {0.5, {kViolet, 27}}};

  for(auto ipad : ROOT::TSeqI(0, 4)){
    trendplot->cd(ipad+1);
    std::map<double, TGraphErrors *> trendgraphs;
    for(auto rad : kJetRadii) {
      auto trendgraph = trending.makeTrend(rad, kTrendPt[ipad]);
      rstyles[rad].SetStyle<TGraphErrors>(*trendgraph);
      trendgraphs.insert({rad, trendgraph});
    }

    auto tmptgraph = trendgraphs.begin()->second;
    (new ROOT6tools::TAxisFrame(Form("trendFrame%s_%s_%d", jettype.data(), trigger.data(), int(kTrendPt[ipad])), 
                                "run", "1/N_{trg} dN/dp_{t,jet} ((GeV/c)^{-1})", 
                                tmptgraph->GetX()[0], tmptgraph->GetX()[tmptgraph->GetN()-1], getmin(tmptgraph), getmax(tmptgraph)))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("p_{t,jet} = %.1f GeV/c", kTrendPt[ipad])))->Draw();
    TLegend *leg(nullptr);
    if(!ipad){
      leg = new ROOT6tools::TDefaultLegend(0.15, 0.8, 0.89, 0.89);
      leg->SetNColumns(4);
      leg->Draw();
    }
    for(auto rad : kJetRadii){
      auto trend = trendgraphs[rad];
      trend->Draw("epsame");
      if(leg) leg->AddEntry(trend, Form("R=%.1f", rad), "lep");
    }
  }
  trendplot->cd();
  trendplot->Update();
  trendplot->SaveCanvas(trendplot->GetName());
}