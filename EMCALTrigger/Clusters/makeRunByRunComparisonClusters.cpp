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
#include <TObjArray.h>
#include <TPaveText.h>
#include <TString.h>
#include <TSystem.h>
#include <AliEmcalList.h>
#endif

#include "../../helpers/graphics.C"


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

std::vector<int> GetListOfRuns(const std::string_view inputdir, const std::string_view tag) {
  std::string runsstring = gSystem->GetFromPipe("ls -1 | grep 000 | sort").Data();
  std::vector<int> runlist;
  std::stringstream runparser(runsstring);
  std::string tmp;
  while(std::getline(runparser, tmp, '\n')){
    auto content = lsdir(tmp);
    if(std::find(content.begin(), content.end(), Form("ClusterSpectra_%s.root", tag.data())) != content.end()){
      if(CheckFileGood(std::string(inputdir) + "/" + tmp + Form("/ClusterSpectra_%s.root", tag.data()))) runlist.emplace_back(std::stoi(tmp));
    }
  }
  std::sort(runlist.begin(), runlist.end(), std::less<int>());
  return runlist; 
}


TH1 *GetNormalizedSpectrum(const std::string_view filename, const std::string_view trigger, const std::string_view cluster) {
  bool isEMCAL = trigger[0] == 'E';
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  reader->cd(cluster.data());
  std::stringstream histname;
  histname << "clusterSpectrum_" << (isEMCAL ? "EMCAL" : "DCAL") << "_";
  if(trigger.find("MB") != std::string::npos){
    histname << "MB";
  } else {
    histname << trigger;
  }
  histname << "_" << cluster;
  auto patches = static_cast<TH1 *>(gDirectory->Get(histname.str().data()));
  patches->SetDirectory(nullptr);
  return patches;
}

TCanvas *DrawRuns(std::vector<int> runrange, const std::string_view inputdir, const std::string_view trigger, const std::string_view cluster, const std::string_view tag, int index){
  auto plot = new TCanvas(Form("runByRunComparisonClusters%s%s%s%d", tag.data(), trigger.data(), cluster.data(), index), Form("comparison runs %d trigger %s in cluster ", index, trigger.data()), 800, 600);
  plot->SetLogy();

  bool isEJ = trigger.find("EJ") != std::string::npos;
  auto frame = new TH1F(Form("frame%s%d", trigger.data(), index), "; E_{cl} (GeV); 1/N_{ev} dN_{cl}/dE (GeV^{-1})", 100, 0., 100.);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(1e-6, 100.);
  frame->Draw();

  auto leg = new TLegend(0.55, 0.4, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->Draw();

  auto label = new TPaveText(0.55, 0.3, 0.89, 0.4, "NDC");
  InitWidget<TPaveText>(*label);
  label->AddText(Form("Trigger: %s", trigger.data()));
  label->Draw();
  
  std::array<Style, 10> styles = {{{kRed, 24}, {kBlue, 25}, {kGreen, 26}, {kMagenta, 27}, {kAzure, 28}, {kOrange, 29}, {kGray, 30},
                                   {kViolet, 31}, {kTeal, 32}, {kBlack, 33}}};
  int counter(0);
  for(auto r : runrange){
    std::cout << "Getting the data for run " << r << std::endl;
    auto hist = GetNormalizedSpectrum(Form("%s/%09d/ClusterSpectra_%s.root", inputdir.data(), r, tag.data()), trigger, cluster);
    styles[counter++].SetStyle<TH1>(*hist);
    hist->SetName(Form("%d", r));
    hist->Draw("epsame");
    leg->AddEntry(hist, hist->GetName(), "lep");
  }
  plot->Update();
  return plot;
}

void makeRunByRunComparisonClusters(const std::string_view trigger, const std::string_view cluster, const std::string_view tag, std::string inputdir = ""){
  std::array<std::string, 5> formats = {{"eps", "pdf", "gif", "png", "jpg"}};
  std::cout << "Finding runs in input dir " << inputdir << " ... " << std::endl;
  if(!inputdir.length()) inputdir = gSystem->GetWorkingDirectory();
  auto runs = GetListOfRuns(inputdir, tag);
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
    auto plot = DrawRuns(runlistPanel, inputdir, trigger, cluster, tag, panels);
    for(auto f : formats) {
      std::stringstream outname;
      outname << plot->GetName() << "." << f;
      plot->SaveAs(outname.str().data());
    }
  }
}