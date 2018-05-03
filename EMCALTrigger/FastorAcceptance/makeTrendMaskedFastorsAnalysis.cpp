#ifndef __CLING__
#include <algorithm>
#include <array>
#include <bitset>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TLegend.h>
#include <TMath.h>
#include <TString.h>
#include <TSystem.h>

#include "AliCDBManager.h"
#include "AliCDBEntry.h"
#include "AliEMCALTriggerDCSConfig.h"
#include "AliEMCALTriggerSTUDCSConfig.h"
#include "AliEMCALTriggerTRUDCSConfig.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/string.C"

class Runlists { 
public:
  Runlists() = default;
  ~Runlists() = default;

  void Initialize(const std::string_view inputdir) {
    fRunlists.clear();
    for(auto d : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data())){
      if(d.find("LHC") == std::string::npos) continue;
      std::vector<int> runs;
      std::ifstream reader(Form("%s/%s", inputdir.data(), d.data()));
      std::string tmp;
      while(getline(reader, tmp)){
        if(!tmp.length()) continue;
        for(auto runstring : tokenize(tmp, ',')) {
          if(!is_number(runstring)) continue;
          runs.emplace_back(std::stoi(runstring));
        }
      }
      std::sort(runs.begin(), runs.end(), std::less<int>());
      fRunlists.insert({d, runs});
    }
  }

  std::string getPeriod(int runnumber) {
    std::string result = "";
    for(auto p : fRunlists) {
      const auto &runlist = p.second;
      if(runnumber >= runlist.front() && runnumber <= runlist.back()) {
        result = p.first;
        break;
      }
    }
    return result;
  }

  bool findRun(int runnumber){
    auto period = getPeriod(runnumber);
    if(!period.length()) return false;
    std::cout << "Found run in period " << period << std::endl;
    const auto &runlist = fRunlists[period];
    return std::find(runlist.begin(), runlist.end(), runnumber) != runlist.end();
  }

  void Print() {
    for(auto p : fRunlists){
      std::cout << "Period " << p.first << ":" << std::endl;
      std::cout << "=========================" << std::endl;
      bool first(true);
      for(auto r : p.second) {
        if(!first) std::cout << ", ";
        else first = false;
        std::cout << r;
      }
      std::cout << std::endl << std::endl;
    }
  }

private:
  std::map<std::string, std::vector<int>>    fRunlists;
};

struct runinfo {
  int run;
  double fracEMCALL0;
  double fracDCALL0;
  double fracEMCALL1;
  double fracDCALL1;
  double fracEMCALOCDB;
  double fracDCALOCDB; 


  bool operator==(const runinfo &other) const { return run == other.run; }
  bool operator<(const runinfo &other) const { return run < other.run; }
};

std::vector<int> getListOfRuns(const std::string_view inputdir = ""){
  std::vector<int> result;
  for(auto c : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.length() ? inputdir.data() : gSystem->pwd())).Data())){
    if(is_number(c)) result.emplace_back(std::stoi(c));
  } 
  return result;
}

std::pair<int, int> getNumberOfMaskedFastors(const std::string_view textfile) {
  std::ifstream reader(textfile.data());
  std::string tmp;
  int nemcal(0), ndcal(0);
  while(getline(reader, tmp)){
    if(!tmp.length()) continue;
    auto absid = std::stoi(tmp);
    if(absid < 2944) nemcal++;
    else if(absid < 4784) ndcal++;
  }
  return {nemcal, ndcal};
}

std::pair<int, int> getNumberOfMaskedFastorsOCDB(int run){
  AliCDBManager *cdb = AliCDBManager::Instance();
  cdb->SetRun(run);

  auto en = cdb->Get("EMCAL/Calib/Trigger");
  auto trgcfg = static_cast<AliEMCALTriggerDCSConfig *>(en->GetObject());
  auto emcregion = std::bitset<sizeof(int) * 8>(trgcfg->GetSTUDCSConfig(false)->GetRegion()),
       dmcregion = std::bitset<sizeof(int) * 8>(trgcfg->GetSTUDCSConfig(false)->GetRegion());
  int nemcal(0), ndcal(0);
  for(auto itru : ROOT::TSeqI(0, 46)){
    bool isDCAL = itru >= 32;
    if((isDCAL && !dmcregion.test(itru - 32)) || (!isDCAL && !emcregion.test(itru))){
      // TRU dead
      if(isDCAL) ndcal += 92;
      else nemcal += 92;
    } else {
      auto truconf = trgcfg->GetTRUDCSConfig(itru);
      int nmasked(0);
      for(auto ireg : ROOT::TSeqI(0,6)){
        nmasked += std::bitset<sizeof(int) * 8>(truconf->GetMaskReg(ireg)).count();
      }
      if(isDCAL) ndcal += nmasked;
      else nemcal += nmasked; 
    }
  }
  return {nemcal, ndcal};
}

std::vector<TGraph *> convertToGraphs(const std::set<runinfo> &trend){
  std::vector<TGraph *> result = {
    new TGraph, new TGraph, new TGraph, new TGraph, new TGraph, new TGraph
  };
  for(auto p : trend) {
    result[0]->SetPoint(result[0]->GetN(), p.run, p.fracEMCALL0);
    result[1]->SetPoint(result[1]->GetN(), p.run, p.fracDCALL0);
    result[2]->SetPoint(result[2]->GetN(), p.run, p.fracEMCALL1);
    result[3]->SetPoint(result[3]->GetN(), p.run, p.fracDCALL1);
    result[4]->SetPoint(result[4]->GetN(), p.run, p.fracEMCALOCDB);
    result[5]->SetPoint(result[5]->GetN(), p.run, p.fracDCALOCDB);
  }
  return result;
}

void makeTrendMaskedFastorsAnalysis(const std::string_view inputdir = ""){  
  Runlists goodruns;
  goodruns.Initialize("/data1/markus/Fulljets/pp_13TeV/Substructuretree/code/runlists_EMCAL");
  std::cout << "Using the following good runs: " << std::endl;
  goodruns.Print();
  AliCDBManager *cdb = AliCDBManager::Instance();
  cdb->SetDefaultStorage("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/2016/OCDB");
  std::set<runinfo> masks;
  for(auto r : getListOfRuns(inputdir)){
    if(!goodruns.findRun(r)) {
      std::cout << "run " << r << " not good - skipping ..." << std::endl;
      continue;        // Handle only good runs from the EMCAL good runlist 
    }
    std::cout << "Processing run " << r << std::endl;
    auto l0data = getNumberOfMaskedFastors(Form("%09d/maskedFastorsFreq_L0_EG1.txt", r)),
         l1data = getNumberOfMaskedFastors(Form("%09d/maskedFastorsFreq_L1_EG1.txt", r)),
         ocdbdata = getNumberOfMaskedFastorsOCDB(r);
    masks.insert({r, static_cast<double>(l0data.first) / 2944., 
                     static_cast<double>(l0data.second) / 1288., 
                     static_cast<double>(l1data.first) / 2944., 
                     static_cast<double>(l1data.second) / 1288.,
                     static_cast<double>(ocdbdata.first) / 2944., 
                     static_cast<double>(ocdbdata.second) / 1288.});
  }
  
  auto graphs = convertToGraphs(masks);
  int ndiff = graphs[0]->GetX()[graphs[0]->GetN()-1] - graphs[0]->GetX()[0];
  int ndigdiff = getDigits(ndiff), scaler = TMath::Power(10, ndigdiff - 1);
  auto plotmin = (static_cast<int>(graphs[0]->GetX()[0])/scaler) * scaler,
       plotmax = ((static_cast<int>(graphs[0]->GetX()[graphs[0]->GetN()-1])) + 1) * scaler;
  auto framesize = plotmax - plotmin;

  auto plot = new TCanvas("masktrending", "Trending reg mask", 800);
  double framemin = (graphs[0]->GetX()[0] / framesize) * framesize, 
         framemax = framemin + framesize;
  auto frame = new TH1F("trendframe", "; run; frac masked fastors", framesize, framemin, framemax);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(0., 0.4);
  frame->Draw("axis");

  Color_t emcalcolor = kRed, dcalcolor = kBlue;
  Style_t ocdbstyle = 24, l0style = 25, l1style = 26;

  auto leg = new TLegend(0.15, 0.8, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->SetNColumns(3);
  leg->Draw();

  std::array<std::string, 3> source = {{"L0", "L1", "OCDB"}};
  std::array<Style_t, 3> styles = {{l0style, l1style, ocdbstyle}};
  for(auto ig : ROOT::TSeqI(0, 6)){
    Style{(ig % 2) ? dcalcolor : emcalcolor, styles[ig/2]}.SetStyle<TGraph>(*graphs[ig]);
    graphs[ig]->Draw("epsame");
    leg->AddEntry(graphs[ig], Form("%s, %s", (ig % 2) ? "DCAL": "EMCAL", source[ig/2].data()), "lep");
  }
  gPad->Update();
}