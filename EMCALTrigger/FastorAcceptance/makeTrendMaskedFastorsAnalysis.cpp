#ifndef __CLING__
#include <array>
#include <bitset>
#include <fstream>
#include <iostream>
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

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

std::vector<std::string> tokenize(const std::string &strtotok){
  std::stringstream tokenizer(strtotok);
  std::vector<std::string> result;
  std::string tmp;
  while(std::getline(tokenizer, tmp, '\n')) result.emplace_back(tmp);
  return result;
}

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

int getDigits(int number) {
  int ndigits(0);
  for(int i = 0; i < 7; i++){
    if(number / int(TMath::Power(10, i))) ndigits++;
  }
  return ndigits;
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
    bool isDCAL = itru < 32;
    if((isDCAL && dmcregion.test(itru - 32)) || (!isDCAL && emcregion.test(itru))){
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
  AliCDBManager *cdb = AliCDBManager::Instance();
  cdb->SetDefaultStorage("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/2016/OCDB");
  std::set<runinfo> masks;
  for(auto r : getListOfRuns(inputdir)){
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
  auto framesize = TMath::Power(10, getDigits(ndiff)+1);

  auto plot = new TCanvas("masktrending", "Trending reg mask", 800);
  double framemin = (graphs[0]->GetX()[0] / framesize) * framesize, 
         framemax = framemin + framesize;
  auto frame = new TH1F("trendframe", "; run; frac masked fastors", framesize, framemin, framemax);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(0., 0.1);
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
    leg->AddEntry(graphs[ig], Form("%s, %s", (ig % 2) ? "EMCAL": "DCAL", source[ig/2].data()), "lep");
  }
  gPad->Update();
}