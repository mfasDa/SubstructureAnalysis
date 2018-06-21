#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TSystem.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"

struct Trendpoint {
  std::string fPeriod;
  double fRadius;
  double fPt;
  double fVal;
  double fError;

  bool operator==(const Trendpoint &other) const { return fPeriod == other.fPeriod; }
  bool operator<(const Trendpoint &other) const { return fPeriod < other.fPeriod; }
};

struct TrendingCollection {
  std::vector<Trendpoint> fData;

  TH1 *makeTrend(double radius, double pt){
    std::set<Trendpoint> points;
    for(const auto &d : fData) {
      if(d.fRadius == radius && d.fPt == pt) points.insert(d);
    }
    TH1 *result = new TH1F("trend", "trend", points.size(), 0., points.size());
    result->SetDirectory(nullptr);
    int currentbin(1);
    for(const auto &p : points){
      result->GetXaxis()->SetBinLabel(currentbin, p.fPeriod.data());
      result->SetBinContent(currentbin, p.fVal);
      result->SetBinError(currentbin, p.fError);
      currentbin++;
    }
    return result;
  }
};

double getmax(const std::vector<TH1 *> histos){
  auto hist = std::max_element(histos.begin(), histos.end(), [](const TH1 *first, const TH1 *second) {return first->GetMaximum() < second->GetMaximum();});
  return (*hist)->GetMaximum();
}

double getmin(const std::vector<TH1 *> histos){
  auto hist = std::min_element(histos.begin(), histos.end(), [](const TH1 *first, const TH1 *second) {return first->GetMinimum() < second->GetMinimum();});
  return (*hist)->GetMinimum();
}

std::string findDirectory(const TFile &reader, const std::string_view jettype, const std::string_view trigger){
  std::string result;
  for(auto d : *(gDirectory->GetListOfKeys())){
    std::string_view keyname = d->GetName();
    if(keyname.find(jettype) == std::string::npos) continue;
    if(keyname.find(trigger) == std::string::npos) continue;
    result = std::string(keyname); 
  }
  return result;
}

std::vector<TH1 *> readTriggerHistos(const std::string_view filename, const std::string_view jettype, const std::string_view trigger){
  std::vector<TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  auto dirname = findDirectory(*reader, jettype, trigger);
  if(!dirname.length()) return result;
  reader->cd(dirname.data());
  for(auto k : *(gDirectory->GetListOfKeys())){
    auto h = static_cast<TH1 *>(static_cast<TKey *>(k)->ReadObj()); 
    h->SetDirectory(nullptr);
    result.emplace_back(h);
  }
  return result;
}

void makePeriodComparison2017(const std::string_view jettype, const std::string_view trigger, const std::string_view triggercluster){
  std::map<std::string, Style> periods = {
    {"LHC17h", {kRed, 24}}, {"LHC17i", {kBlue, 25}}, {"LHC17j", {kGreen, 26}}, {"LHC17k", {kViolet, 27}}, {"LHC17l", {kTeal, 28}},
    {"LHC17m", {kMagenta, 29}}, {"LHC17o", {kBlack, 30}} 
  };

  auto plot = new ROOT6tools::TSavableCanvas(Form("periodComparison%s%s", jettype.data(), trigger.data()), Form("Period comparison %s in trigger %s", jettype.data(), trigger.data()), 1200, 1000);
  plot->Divide(2,2);

  int ipad = 1;
  TLegend *leg(nullptr);
  for(auto irad : ROOT::TSeqI(2,6)){
    plot->cd(ipad);
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame(Form("JetSpecFrameR%02d%s%s", irad, jettype.data(), trigger.data()), "p_{t,jet} (GeV/c)", "1/N_{trg} dN_{jet}/dp_{t,jet} ((GeV/c)^{-1}", 0., 200., 1e-7, 200))->Draw("axis");

    if(ipad == 1) {
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.45, 0.89, 0.89);
      leg->Draw();

      (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.5, 0.88, Form("%s, trigger %s", jettype.data(), trigger.data())))->Draw();
    }

    (new ROOT6tools::TNDCLabel(0.15, 0.72, 0.25, 0.79, Form("R=%.1f", double(irad)/10.)))->Draw();
    ipad++;
  }

  TrendingCollection trending;
  const std::array<double, 4> kTrendPt = {{20., 40., 60., 100.}};
  for(const auto &p : periods){
    std::stringstream infilename;
    infilename << p.first << "/JetSpectra_" << triggercluster << ".root";
    if(gSystem->AccessPathName(infilename.str().data())) continue;
    auto histos = readTriggerHistos(infilename.str(), jettype, trigger);
    
    ipad = 1;
    for(auto irad : ROOT::TSeqI(2, 6)){
      plot->cd(ipad);
      auto hist = *std::find_if(histos.begin(), histos.end(), [irad](const TH1 *hist) -> bool { 
        std::string_view histname(hist->GetName());
        return histname.find(Form("R%02d", irad)) != std::string::npos && histname.find("JetSpectrum") == 0; 
        });
      p.second.SetStyle<TH1>(*hist);
      hist->Draw("epsame");
      if(ipad == 1) leg->AddEntry(hist, p.first.data(), "lep");
      ipad++;

      // fill trending
      for(auto pt : kTrendPt){
        auto bin = hist->GetXaxis()->FindBin(pt);
        trending.fData.push_back({p.first, double(irad)/10., pt, hist->GetBinContent(bin), hist->GetBinError(bin)});
      }
    }
  }

  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());

  // Create trending plot
  auto trendplot = new ROOT6tools::TSavableCanvas(Form("trending_%s_%s", jettype.data(), trigger.data()), Form("Trending plot jet type %s, trigger %s", jettype.data(), trigger.data()), 1200, 1000);
  trendplot->Divide(2,2);

  std::map<double, Style> radstyles = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, {0.4, {kGreen, 26}}, {0.5, {kViolet, 27}}};
  for(int ipt : ROOT::TSeqI(0, 4)){
    std::vector<TH1 *> trendhistos;
    std::cout << "Doint pt " << kTrendPt[ipt] << std::endl;
    for(auto r : ROOT::TSeqI(2, 6)){
      auto hist = trending.makeTrend(double(r)/10., kTrendPt[ipt]);
      hist->SetName(Form("trend_%s_%s_%d_R%02d", jettype.data(), trigger.data(), int(kTrendPt[ipt]), r));
      radstyles[double(r)/10.].SetStyle<TH1>(*hist);
      trendhistos.emplace_back(hist);
    }

    trendplot->cd(ipt+1);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.06);
    auto frame = static_cast<TH1 *>(trendhistos[0]->Clone());
    frame->SetDirectory(nullptr);
    frame->SetName(Form("trendframe_%s_%s_%d", jettype.data(), trigger.data(), int(kTrendPt[ipt])));
    frame->SetTitle("");
    frame->SetStats(false);
    frame->GetYaxis()->SetTitle("1/N_{trg} dN/dp_{t}");
    frame->GetYaxis()->SetTitleOffset(1.5);
    frame->GetYaxis()->SetRangeUser(0.8 * getmin(trendhistos), 1.2 * getmax(trendhistos));
    frame->Draw("axis");

    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("p_{t,jet} = %.1f GeV/c", kTrendPt[ipt])))->Draw();
    TLegend *leg(nullptr);
    if(ipt == 0) {
      leg = new ROOT6tools::TDefaultLegend(0.15, 0.8, 0.89, 0.89);
      leg->SetNColumns(4);
      leg->Draw();
      (new ROOT6tools::TNDCLabel(0.25, 0.9, 0.55, 0.97, Form("%s, %s", jettype.data(), trigger.data())))->Draw();
    }
    for(auto h : trendhistos){
      h->Draw("epsame");
      if(leg) {
        std::string_view histnamestring(h->GetName());
        auto rstring = histnamestring.substr(histnamestring.find_last_of("_")+2);
        leg->AddEntry(h, Form("R=%.1f", double(std::stoi(std::string(rstring)))/10.), "lep");
      }
    }
  }
  trendplot->cd();
  trendplot->Update();
  trendplot->SaveCanvas(trendplot->GetName());
}