#ifndef __CLING__
#include <algorithm>
#include <array>
#include <bitset>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include <ROOT/TSeq.hxx>
#include <TBox.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TLine.h>

#include "AliCDBEntry.h"
#include "AliCDBManager.h"
#include "AliEMCALGeometry.h"
#include "AliEMCALTriggerDCSConfig.h"
#include "AliEMCALTriggerTRUDCSConfig.h"
#include "AliEMCALTriggerSTUDCSConfig.h"
#endif

#include "../../helpers/graphics.C"

using Range = ROOT::TSeqI;
using TFilePtr = std::unique_ptr<TFile>;

const Color_t         kColAll      = kBlack,
                      kColOCDBL0   = kOrange,
                      kColOCDBL1   = kMagenta,
                      kColL0L1     = kViolet,
                      kColL0       = kGreen,
                      kColL1       = kBlue,
                      kColOCDB     = kRed;

AliEMCALGeometry *egeo(nullptr);

int GetTRUChannelRun2(int ifield, int ibit){
      const int kChannelMap[6][16] = {{ 8, 9,10,11,20,21,22,23,32,33,34,35,44,45,46,47},   // Channels in mask0
                                      {56,57,58,59,68,69,70,71,80,81,82,83,92,93,94,95},   // Channels in mask1
                                      { 4, 5, 6, 7,16,17,18,19,28,29,30,31,40,41,42,43},   // Channels in mask2
                                      {52,53,54,55,64,65,66,67,76,77,78,79,88,89,90,91},   // Channels in mask3
                                      { 0, 1, 2, 3,12,13,14,15,24,25,26,27,36,37,38,39},   // Channels in mask4
                                      {48,49,50,51,60,61,62,63,72,73,74,75,84,85,86,87}};  // Channels in mask5
      return kChannelMap[ifield][ibit];
}

int GetTRUChannelRun1(int ifield, int ibit){  
  return ifield * 16 + ibit;
}

int RemapTRUIndex(int itru) {
  int map[46] = {0,1,2,5,4,3,6,7,8,11,10,9,12,13,14,17,16,15,18,19,20,23,22,21,24,25,26,29,28,27,30,31,32,33,37,36,38,39,43,42,44,45,49,48,50,51};
  return map[itru];
}

std::set<int> ReadMaskedFastorsOCDB(int runnumber){
  std::set<int> fastorabsids;
  auto cdb = AliCDBManager::Instance();
  cdb->SetRun(runnumber);

  auto en = cdb->Get("EMCAL/Calib/Trigger");
  auto trgcfg = static_cast<AliEMCALTriggerDCSConfig *>(en->GetObject());
  std::bitset<sizeof(int) *8> emcalregion(trgcfg->GetSTUDCSConfig(false)->GetRegion()), dcalregion(trgcfg->GetSTUDCSConfig(true)->GetRegion());
  int nmaskedEMCAL(0), nmaskedDCAL(0);
  for(auto itru : Range(0, 46)) {
    bool isDCAL = itru >= 32;
    if((isDCAL && !dcalregion.test(itru-32)) || (!isDCAL && !emcalregion.test(itru))){
      // TRU dead - mark all channels of the TRU as masked
      for(auto ichan : ROOT::TSeqI(0, 96)) {
        int fastOrAbsID(-1);
        egeo->GetTriggerMapping()->GetAbsFastORIndexFromTRU(RemapTRUIndex(itru), ichan, fastOrAbsID);
        fastorabsids.insert(fastOrAbsID);
      }
    } else {
      // not dead
      auto truconf = trgcfg->GetTRUDCSConfig(itru);
      for(auto imask : Range(0, 6)){
        auto regmask = truconf->GetMaskReg(imask);
        std::bitset<sizeof(decltype(regmask)) * 8> mask(regmask);
        for(auto ibit : Range(0, mask.size())){
          if(mask.test(ibit)){
            if(itru < 32) nmaskedEMCAL++;
            else nmaskedDCAL++;
            auto chantru = (itru == 30 || itru == 31 || itru == 44 || itru == 45) ? GetTRUChannelRun1(imask, ibit) : GetTRUChannelRun2(imask, ibit);  
            int fastOrAbsID(-1);
            egeo->GetTriggerMapping()->GetAbsFastORIndexFromTRU(RemapTRUIndex(itru), chantru, fastOrAbsID);
            fastorabsids.insert(fastOrAbsID);
          }
        }
      }
    }
  }
  std::cout << "Found " << nmaskedEMCAL << " channels in EMCAL and " << nmaskedDCAL << " channels in DCAL." << std::endl;
  return fastorabsids;
}

std::set<int> ReadMaskedFastorsFile(const char *textfile){
  std::set<int> fastorabsids;
  std::ifstream reader(textfile);
  std::string tmp;
  while(getline(reader, tmp)) fastorabsids.insert(std::stoi(tmp));
  return fastorabsids;
}

void DrawFastOr(int col, int row, Color_t mycolor){
  TBox *result = new TBox(col, row, col+1, row+1);
  result->SetLineWidth(0);
  result->SetFillColor(mycolor);
  result->Draw();
}

void DrawAllFastOrs(const std::set<int> &fastors, Color_t mycolor) {
  for(auto fastor : fastors) {
    int mycol, myrow;
    egeo->GetTriggerMapping()->GetPositionInEMCALFromAbsFastORIndex(fastor, mycol, myrow);
    DrawFastOr(mycol, myrow, mycolor);
  }
}

TCanvas *PlotMaskedChannels(const std::set<int> &ocdb, const std::set<int> &l0, const std::set<int> &l1){
  // first three helper functions to draw the
  // of course in functional style (lambda functions)
  std::function<void()> DrawSupermoduleGrid = [](){
    TLine *l(nullptr);
    // EMCAL
    for(int i = 12; i <= 60; i += 12) {
      l = new TLine(0, i, 48, i);
      l->SetLineWidth(2);
      l->Draw();
    }
    l = new TLine(24, 0, 24, 64);
    l->SetLineWidth(2);
    l->Draw();
    l = new TLine(0, 64, 48, 64);
    l->SetLineWidth(2);
    l->Draw();
    //DCAL
    for(int i = 76; i < 100; i+=12){
      l = new TLine(0, i, 16, i);
      l->SetLineWidth(2);
      l->Draw();
      l = new TLine(32, i, 48, i);
      l->SetLineWidth(2);
      l->Draw();
    }
    l = new TLine(16, 64, 16, 100);
    l->SetLineWidth(2);
    l->Draw();
    l = new TLine(32, 64, 32, 100);
    l->SetLineWidth(2);
    l->Draw();
    l = new TLine(0, 100, 48, 100);
    l->SetLineWidth(2);
    l->Draw();
    l = new TLine(24, 100, 24, 104);
    l->SetLineWidth(2);
    l->Draw();
  };

  std::function<void()> DrawTRUGrid = [](){
    TLine *l(nullptr);
    // EMCAL
    for(int r = 0; r < 60; r += 12) {
      for(int c = 0; c < 48;  c += 8 ){
        if(c == 0 || c == 24) continue;
        l = new TLine(c, r, c, r + 12);
        l->SetLineWidth(1);
        l->SetLineStyle(2);
        l->Draw();
      }
    }
    // DCAL
    std::array<int, 2> colsdcal = {{8, 40}};
    for(int r = 64; r < 100; r += 12){
      for(auto c : colsdcal) {
        l = new TLine(c, r, c, r + 12);
        l->SetLineWidth(1);
        l->SetLineStyle(2);
        l->Draw();
      }
    }
  };


  TCanvas *result = new TCanvas("maskedFastorGrid", "Masked Fastors", 800, 600);
  result->cd();
  gPad->SetRightMargin(0.25);

  TH1 *axis = new TH1F("axis", "Masked FastORs; col; row", 48, 0., 48.);
  axis->SetStats(false);
  axis->SetDirectory(nullptr);
  axis->GetYaxis()->SetRangeUser(0, 104);
  axis->Draw("axis");

  DrawSupermoduleGrid();
  DrawTRUGrid();

  std::set<int> channelsAll, channelsOCDBL0, channelsOCDBL1, channelsL0L1, channelsOCDB, channelsL0, channelsL1,
                l0work = l0, l1work = l1;
  for(auto d : ocdb){
    auto l0pos = l0work.find(d), l1pos = l1work.find(d);
    if(l0pos != l0work.end()){
      if(l1pos != l1work.end()) {
        channelsAll.insert(d);
        l0work.erase(l0pos);
        l1work.erase(l1pos);
      } else {
        channelsOCDBL0.insert(d);
        l0work.erase(l0pos);
      }
    } else if(l1pos != l1work.end()) {
      channelsOCDBL1.insert(d);
    } else {
      channelsOCDB.insert(d);
    }
  }
  for(auto d : l0work) {
    auto l1pos = l1work.find(d);
    if(l1pos != l1work.end()) {
      channelsL0L1.insert(d);
      l1work.erase(l1pos);
    } else {
      channelsL0.insert(d);
    }
  }
  for(auto d : l1work) {
    channelsL1.insert(d);
  }

  DrawAllFastOrs(channelsAll, kColAll);
  DrawAllFastOrs(channelsOCDBL0, kColOCDBL0);
  DrawAllFastOrs(channelsOCDBL1, kColOCDBL1);
  DrawAllFastOrs(channelsL0L1, kColL0L1);
  DrawAllFastOrs(channelsOCDB, kColOCDB);
  DrawAllFastOrs(channelsL0, kColL0);
  DrawAllFastOrs(channelsL1, kColL1);

  TLegend *leg = new TLegend(0.75, 0.6, 0.99, 0.89);
  InitWidget<TLegend>(*leg);  
  auto *ball = new TBox, *bocdbl0 = new TBox, *bocdbl1 = new TBox,
       *bl0l1 = new TBox, *bocdb = new TBox, *bl0 = new TBox, *bl1 = new TBox;
  ball->SetFillColor(kColAll);
  bocdbl0->SetFillColor(kColOCDBL0);
  bocdbl1->SetFillColor(kColOCDBL1);
  bl0l1->SetFillColor(kColL0L1);
  bocdb->SetFillColor(kColOCDB);
  bl0->SetFillColor(kColL0);
  bl1->SetFillColor(kColL1);
  leg->AddEntry(ball, Form("all: %d", int(channelsAll.size())), "f");
  leg->AddEntry(bocdbl0, Form("OCDB+L0: %d", int(channelsOCDBL0.size())), "f");
  leg->AddEntry(bocdbl1, Form("OCDB+L1: %d", int(channelsOCDBL1.size())), "f");
  leg->AddEntry(bl0l1, Form("L0+L1: %d", int(channelsL0L1.size())), "f");
  leg->AddEntry(bocdb, Form("OCDB: %d", int(channelsOCDB.size())), "f");
  leg->AddEntry(bl0, Form("L0: %d", int(channelsL0.size())), "f");
  leg->AddEntry(bl1, Form("L1: %d", int(channelsL1.size())), "f");
  leg->Draw();

  std::cout << "Number of dead channels in ALL methods:" << channelsAll.size() << std::endl; 
  std::cout << "Number of dead channels in OCDB and L0:" << channelsOCDBL0.size() << std::endl; 
  std::cout << "Number of dead channels in OCDB and L1:" << channelsOCDBL1.size() << std::endl; 
  std::cout << "Number of dead channels in L0 and L1:  " << channelsL0L1.size() << std::endl; 
  std::cout << "Number of dead channels in OCDB only:  " << channelsOCDB.size() << std::endl; 
  std::cout << "Number of dead channels in L0 only:    " << channelsL0.size() << std::endl; 
  std::cout << "Number of dead channels in L1 only:    " << channelsL1.size() << std::endl; 

  return result;
}

void SaveCanvas(const std::string &basename, const TCanvas *plot){
  std::vector<std::string> endings = {"eps", "pdf", "png", "jpeg", "gif"};
  for(auto e : endings) plot->SaveAs(Form("%s.%s", basename.c_str(), e.c_str()));
}

void visualizeMaskComparisonRun(int runnumber){
  int year=2015;
  if(runnumber <= 247170) year = 2015;
  else if(runnumber <= 267254) year = 2016;
  else if(runnumber <= 282900) year = 2017;
  else year = 2018;
  egeo = AliEMCALGeometry::GetInstanceFromRunNumber(runnumber);
  AliCDBManager::Instance()->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", year)); 
  SaveCanvas(Form("compMaskedFastors_%d", runnumber), PlotMaskedChannels(ReadMaskedFastorsOCDB(runnumber), ReadMaskedFastorsFile("maskedFastorsFreq_L0_EG1.txt"), ReadMaskedFastorsFile("maskedFastorsFreq_L1_EG1.txt")));
}
