#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#include <TMath.h>
#include <TObjArray.h>
#include <TPaveText.h>
#include <TString.h>
#include <TSystem.h>
#include <AliEmcalList.h>
#endif

#include "../helpers/graphics.C"

struct Point{
  int run;
  double mean;
  double rms;

  bool operator<(const Point &other) const { return run < other.run; }
};

std::set<Point> trend;

std::vector<std::string> lsdir(const std::string_view inputdir) {
  std::string dircontent = gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data();
  std::stringstream parser(dircontent);
  std::string tmp;
  std::vector<std::string> result;
  while(std::getline(parser, tmp, '\n')) result.emplace_back(tmp);
  return result;
}

bool CheckFileGood(const std::string_view filename) {
  std::cout << "Checking " << filename << std::endl;
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  if(!reader.get() || reader->IsZombie()) return false;
  if(!gDirectory->GetListOfKeys()->GetEntries()) return false;
  reader->Close();
  return true;
}

std::vector<int> GetListOfRuns(const std::string_view inputdir, const std::string_view cluster) {
  std::string runsstring = gSystem->GetFromPipe("ls -1 | grep 000 | sort").Data();
  std::vector<int> runlist;
  std::stringstream runparser(runsstring);
  std::string tmp;
  while(std::getline(runparser, tmp, '\n')){
    auto content = lsdir(tmp);
    if(std::find(content.begin(), content.end(), Form("PatchADCSpectra_%s.root", cluster.data())) != content.end()){
      if(CheckFileGood(std::string(inputdir) + "/" + tmp + Form("/PatchADCSpectra_%s.root", cluster.data()))) runlist.emplace_back(std::stoi(tmp));
    }
  }
  std::sort(runlist.begin(), runlist.end(), std::less<int>());
  return runlist; 
}

TH1 *GetNormalizedRatioSpectrum(const std::string_view filename, const std::string_view cluster) {
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  std::stringstream histnameEG1, histnameEG2;
  histnameEG1 << "hPatchADCEGAEG1" << cluster;
  histnameEG2 << "hPatchADCEGAEG2" << cluster;
  auto EG1 = static_cast<TH1 *>(reader->Get(histnameEG1.str().data())),
       EG2 = static_cast<TH1 *>(reader->Get(histnameEG2.str().data()));
  EG1->SetDirectory(nullptr);
  EG1->Rebin(4);
  EG2->Rebin(4);
  EG1->Divide(EG2);
  return EG1;
}

std::pair<double, double> GetMeanWidth(TH1 *histo, double ADCmin, double ADCmax) {
  std::vector<double> ratiovals;
  for(auto b : ROOT::TSeqI(histo->GetXaxis()->FindBin(ADCmin), histo->GetXaxis()->FindBin(ADCmax) + 1)){
    ratiovals.emplace_back(histo->GetBinContent(b));
  }
  return std::pair<double, double>(TMath::Mean(ratiovals.begin(), ratiovals.end()), TMath::RMS(ratiovals.begin(), ratiovals.end()));
}

TCanvas *DrawRuns(std::vector<int> runrange, const std::string_view inputdir, const std::string_view cluster, int index){
  auto plot = new TCanvas(Form("runByRunComparisonREGARecalcPatches%s%d", cluster.data(), index), Form("comparison runs %d cluster ", index), 800, 600);

  auto frame = new TH1F(Form("frame%d",index), "; ADC; EG1/EG2", 100, 0., 300.);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(0, 25.);
  frame->Draw();

  auto leg = new TLegend(0.15, 0.4, 0.39, 0.89);
  InitWidget<TLegend>(*leg);
  leg->Draw();

  std::array<Style, 10> styles = {{{kRed, 24}, {kBlue, 25}, {kGreen, 26}, {kMagenta, 27}, {kAzure, 28}, {kOrange, 29}, {kGray, 30},
                                   {kViolet, 31}, {kTeal, 32}, {kBlack, 33}}};
  int counter(0);
  for(auto r : runrange){
    std::cout << "Getting the data for run " << r << std::endl;
    auto hist = GetNormalizedRatioSpectrum(Form("%s/%09d/PatchADCSpectra_%s.root", inputdir.data(), r, cluster.data()), cluster);
    styles[counter++].SetStyle<TH1>(*hist);
    hist->SetName(Form("%d", r));
    hist->Draw("epsame");
    leg->AddEntry(hist, hist->GetName(), "lep");
    auto quantiles = GetMeanWidth(hist, 120, 250);
    trend.insert({r, quantiles.first, quantiles.second}); 
  }
  plot->Update();
  return plot;
}

void makeRunByRunComparisonREGARecalcPatches(const std::string_view cluster, std::string inputdir = ""){
  std::array<std::string, 5> formats = {{"eps", "pdf", "gif", "png", "jpg"}};
  std::cout << "Finding runs in input dir " << inputdir << " ... " << std::endl;
  if(!inputdir.length()) inputdir = gSystem->GetWorkingDirectory();
  auto runs = GetListOfRuns(inputdir, cluster);
  std::cout << "Found " << runs.size() << " runs ..." << std::endl;
  auto ncanvas  = runs.size()/10;
  if(runs.size() % 10) ncanvas++;
  int current  = 0;
  for(auto panels : ROOT::TSeqI(0, ncanvas)){
    std::vector<int> runlistPanel;
    int nrunsproc = 0;
    for(auto runiter : ROOT::TSeqI(0, 10)){
      if(current == runs.size()) break;
      runlistPanel.emplace_back(runs[current++]);
      nrunsproc++;
    }
    if(!nrunsproc) break;
    auto plot = DrawRuns(runlistPanel, inputdir, cluster, panels);
    for(auto f : formats) {
      std::stringstream outname;
      outname << plot->GetName() << "." << f;
      plot->SaveAs(outname.str().data());
    }
  }


  auto trendgraph = new TGraphErrors;
  int nrun(0);
  double rmin = -1, rmax;
  for(auto r : trend) {
    trendgraph->SetPoint(nrun, r.run, r.mean);
    trendgraph->SetPointError(nrun, 0, r.rms);
    nrun++;
    if(rmin<0) rmin = r.run;
    rmax = r.run;
  }
  trendgraph->SetMarkerColor(kBlack);
  trendgraph->SetLineColor(kBlack);
  trendgraph->SetMarkerStyle(25);

  auto trendplot = new TCanvas("trendplot", "trendplot EG1/EG2", 800, 600);
  auto trendframe = new TH1F("trendframe", "; run; <EG1/EG2>", rmax-rmin, rmin, rmax);
  trendframe->SetDirectory(nullptr);
  trendframe->SetStats(false);
  trendframe->GetYaxis()->SetRangeUser(0, 25.);
  trendframe->Draw("axis");
  trendgraph->Draw("elpsame");

  for(auto f : formats) {
    trendplot->SaveAs(Form("trendEG1EG2.%s", f.data()));
  }
}