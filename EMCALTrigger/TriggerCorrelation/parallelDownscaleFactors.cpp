#ifndef __CLING__
#include <iostream>
#include <map>
#include <set>
#include <string>

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

struct RunDownscale : public TObject{
public:
  RunDownscale() = default;
  RunDownscale(int run, std::map<std::string, double> data) : fRun(run), fData(data) { }
  virtual ~RunDownscale() = default;
  
  bool operator==(const RunDownscale &other) const { return fRun == other.fRun; }
  bool operator<(const RunDownscale &other) const { return fRun < other.fRun; }

  Int_t Run() const { return fRun; }
  std::map<std::string, double> &Data() { return fData; }

  virtual Int_t Compare(const TObject *other) const final{
    RunDownscale *otherrun = (RunDownscale *)other;
    if(*this == *otherrun) return 0;
    else if(*this < *otherrun) return -1;
    else return 1;
  }
  Bool_t IsEqual(const TObject *other) const final{
    RunDownscale *otherrun = (RunDownscale *)other;
    return *this == *otherrun;
  }
  Bool_t IsSortable() const final { return kTRUE; }

private:
  int                             fRun;
  std::map<std::string, double>   fData;

};

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
  auto runlist = getListOfRuns(basedir);

  const int NWORKERS = 10;
  auto workitem = [&](int workerID) {
    TSortedList *result = new TSortedList;
    int idiv = 0;
    while(true){
      auto runid = NWORKERS * idiv + workerID;
      if(runid >= runlist.size()) break;
      auto myrun = runlist[runid];
      auto runds = getDownscaleFactorForRun(myrun);
      result->Add(new RunDownscale(myrun, runds));
      std::cout << "Current number of entries in pool: " << result->GetEntries() << std::endl;
      idiv += 1;
    }
    return result;
  };
  ROOT::TProcessExecutor processor(NWORKERS);
  ROOT::ExecutorUtils::ReduceObjects<TSortedList *> redfun;
  auto rundata = processor.MapReduce(workitem, ROOT::TSeqI(0, NWORKERS), redfun);

  TGraph * geg2 = new TGraph;
  for(auto r : *rundata){
    auto robject = static_cast<RunDownscale *>(r);
    geg2->SetPoint(geg2->GetN(), robject->Run(), robject->Data()["EG2"]);
  }
  geg2->Draw("ape");
}