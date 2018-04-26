#ifndef __CLING__
#include <algorithm>
#include <bitset>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TString.h>
#include <TSystem.h>
  
#include <AliCDBEntry.h>
#include <AliCDBManager.h>
#include <AliEMCALGeometry.h>
#include <AliEMCALTriggerDCSConfig.h>
#include <AliEMCALTriggerTRUDCSConfig.h>
#endif

AliEMCALGeometry *ggeo = nullptr;

int GetTRUChannelRun2(int ifield, int ibit){
      const int kChannelMap[6][16] = {{ 8, 9,10,11,20,21,22,23,32,33,34,35,44,45,46,47},   // Channels in mask0
                                      {56,57,58,59,68,69,70,71,80,81,82,83,92,93,94,95},   // Channels in mask1
                                      { 4, 5, 6, 7,16,17,18,19,28,29,30,31,40,41,42,43},   // Channels in mask2
                                      {52,53,54,55,64,65,66,67,76,77,78,79,88,89,90,91},   // Channels in mask3
                                      { 0, 1, 2, 3,12,13,14,15,24,25,26,27,36,37,38,39},   // Channels in mask4
                                      {48,49,50,51,60,61,62,63,72,73,74,75,84,85,86,87}};  // Channels in mask5
      return kChannelMap[ifield][ibit];
}

int RemapTRUIndex(int itru) {
  int map[46] = {0,1,2,5,4,3,6,7,8,11,10,9,12,13,14,17,16,15,18,19,20,23,22,21,24,25,26,29,28,27,30,31,32,33,37,36,38,39,43,42,44,45,49,48,50,51};
  return map[itru];
}

std::vector<int> GetMaskedFastorsFromOCDB(int run){
  if(!ggeo) ggeo = AliEMCALGeometry::GetInstanceFromRunNumber(run);
  std::vector<int> result;

  AliCDBManager *cdb = AliCDBManager::Instance();
  cdb->SetRun(run);
  auto trgdcs = static_cast<AliEMCALTriggerDCSConfig *>(cdb->Get("EMCAL/Calib/Trigger")->GetObject());
  for(auto itru : ROOT::TSeqI(0, 46)){
    auto truconf = trgdcs->GetTRUDCSConfig(itru);
    for(auto imask : ROOT::TSeqI(0, 6)){
      auto regmask = truconf->GetMaskReg(imask);
      std::bitset<sizeof(decltype(regmask)) * 8> mask(regmask);
      for(auto ibit : ROOT::TSeqI(0, mask.size())){
        if(mask.test(ibit)){
          auto chantru = GetTRUChannelRun2(imask, ibit);
          int fastOrAbsID(-1);
          ggeo->GetTriggerMapping()->GetAbsFastORIndexFromTRU(RemapTRUIndex(itru), chantru, fastOrAbsID);
          result.emplace_back(fastOrAbsID);
        }
      }
    }
  }
  std::sort(result.begin(), result.end(), std::less<int>());
  return result;
}

std::vector<int> GetMaskedFastorsFromAnalysis(int run){
  std::vector<int> result;
  std::ifstream reader(Form("%09d/maskedFastors.txt", run));
  std::string tmp;
  std::vector<std::pair<int, int>> phosholes = {{3264, 3456}, {3840, 4032}, {4416, 4607}};
  while(getline(reader, tmp)){
    //std::cout << tmp << std::endl;
    int channel = std::stoi(tmp);
    // check for phos hole
    bool isPHOShole = false;
    for(int iphos = 0; iphos < 3; iphos++) {
      if(channel >= phosholes[iphos].first && channel < phosholes[iphos].second){
        isPHOShole = true;
        break;
      }
    }
    if(isPHOShole) continue;
    result.emplace_back(channel);
  }
  std::sort(result.begin(), result.end(), std::less<int>());
  return result;
}

std::vector<std::string> listdir(const std::string_view inputdir){
  std::string dirstring = gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data();
  std::vector<std::string> dircontent;
  std::stringstream tokenizer(dirstring);
  std::string tmp;
  while(std::getline(tokenizer, tmp, '\n')) dircontent.emplace_back(tmp); 
  return dircontent;
}

bool is_number(const std::string& s){
  return !s.empty() && std::find_if(s.begin(), 
    s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

std::vector<int> GetListOfRuns(const std::string_view inputdir){
  std::vector<int> runs;
  for(auto r : listdir(inputdir)){
    if(is_number(r)) runs.emplace_back(std::stoi(r));
  }
  return runs;
}

bool sameMaskLists(std::vector<int> ocdb, std::vector<int> analysis){
  // return true if sets are equal
  int nmissocdb(0), nmissanalysis(0);
  for(auto id : analysis) {
    if(std::find(ocdb.begin(), ocdb.end(), id) == ocdb.end()) {
      std::cout << "Dead FastOR " << id << " not masked in OCDB" << std::endl;
      nmissocdb++;
    }
  }
  for(auto id : ocdb){
    if(std::find(analysis.begin(), analysis.end(), id) == analysis.end()){
      std::cout << "Non-zero spectrum for masked FastOR " << id << std::endl;
      nmissanalysis++;
    }
  }
  return !(nmissocdb || nmissanalysis);
}

void PrintFastorAbsIDs(std::vector<int> fabsIDS){
  for(auto i : fabsIDS) std::cout << i << ", " << std::endl;
}

void dumpList(const std::string &outputfile, const std::vector<int> &runs){
  std::ofstream writer(outputfile);
  for(auto r : runs) writer << r << std::endl;
  writer.close();
}

void runComparisonFastorAcceptance(){
  AliCDBManager *cdb = AliCDBManager::Instance();
  cdb->SetDefaultStorage("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/2016/OCDB");

  std::vector<int> goodruns, badruns;
  for(auto r  : GetListOfRuns(gSystem->pwd())){
    std::cout << "Testing run " << r << std::endl;
    auto fastorsOCDB = GetMaskedFastorsFromOCDB(r),
         fastorsAnalysis  = GetMaskedFastorsFromAnalysis(r);
    std::cout << "FastORs in OCDB: " << std::endl;
    PrintFastorAbsIDs(fastorsOCDB);
    if(!sameMaskLists(fastorsOCDB, fastorsAnalysis)) {
      std::cout << "Mismatch between dead FastOR lists" << std::endl;
      badruns.emplace_back(r);
    } else {
      goodruns.emplace_back(r);
    }
    break;
  }

  dumpList("goodruns.txt", goodruns);
  dumpList("badruns.txt", badruns);
}