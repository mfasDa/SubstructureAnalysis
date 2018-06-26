#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>

#include <ROOT/TProcessExecutor.hxx>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TGraph.h>
#include <TSortedList.h>

#include <AliCDBManager.h>
#include <AliEmcalDownscaleFactorsOCDB.h>
#endif

#include "../../helpers/cdb.C"
#include "../../helpers/string.C"

std::map<std::string, double> getDownscaleFactorForRun(double run){
  std::map<std::string, double> result;
  auto cdb = AliCDBManager::Instance();
  if(!cdb->IsDefaultStorageSet()) cdb->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", getYearForRunNumber(run))); 
  cdb->SetRun(run);
  PWG::EMCAL::AliEmcalDownscaleFactorsOCDB *downscalehandler = PWG::EMCAL::AliEmcalDownscaleFactorsOCDB::Instance();
  downscalehandler->SetRun(run);

  std::map<std::string, std::string> triggers = {{"EG2", "CEMC7EG2-B-NOPF-CENT"}, {"EJ2", "CEMC7EJ2-B-NOPF-CENT"}, 
                                                 {"DG2", "CDMC7DG2-B-NOPF-CENT"}, {"DJ2", "CDMC7DJ2-B-NOPF-CENT"}};
  for(auto t : triggers) {
    auto weight = downscalehandler->GetDownscaleFactorForTriggerClass(t.second.data());
    std::cout << "Found weight " << weight << " for trigger " << t.first << " (" << t.second << ")" << std::endl;
    if(weight) result[t.first] = weight;
  }

  return result; 
}

std::vector<int> getListOfRuns(const std::string_view basedir){
  std::vector<int> runs;  
  for(auto d : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", basedir.data())).Data())){
    if(is_number(d)) runs.emplace_back(std::stoi(d));
  }
  return runs;
}

void parallelDownscaleFactors(const std::string_view basedir){
  using rundata = std::tuple<int, std::map<std::string, double>>;
  auto runlist = getListOfRuns(basedir);

  const int NWORKERS = 10;
  auto workitem = [&](int workerID) {
    std::vector<rundata> result;
    int idiv = 0;
    while(true){
      auto runid = NWORKERS * idiv + workerID;
      if(runid >= runlist.size()) break;
      auto myrun = runlist[runid];
      auto runds = getDownscaleFactorForRun(myrun);
      result.push_back(std::make_tuple(myrun, runds));
      idiv += 1;
    }
    return result;
  };
  
  auto reducer = [](const std::vector<std::vector<rundata>> in){
    std::vector<rundata> result;
    for(auto s : in){
      for(auto o : s){
        result.push_back(o);
      }
    }
    std::sort(result.begin(), result.end(), [](const rundata &first, const rundata &second) { return std::get<0>(first) < std::get<0>(second);});
    return result;
  };
  ROOT::TProcessExecutor processor(NWORKERS);
  //ROOT::ExecutorUtils::ReduceObjects<TSortedList *> redfun;
  auto runresult = processor.MapReduce(workitem, ROOT::TSeqI(0, NWORKERS), reducer);

  TGraph * geg2 = new TGraph;
  for(auto r : runresult){
    geg2->SetPoint(geg2->GetN(), std::get<0>(r), std::get<1>(r)["EG2"]);
  }
  geg2->Draw("ape");
}