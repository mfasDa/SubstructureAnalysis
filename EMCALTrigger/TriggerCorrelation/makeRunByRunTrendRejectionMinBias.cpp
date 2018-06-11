#ifndef __CLING__
#include <iostream>
#include <map>
#include <memory>
#include <ROOT/TSeq.h>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>
#include <TSystem.h>

#include "AliCDBManager.h"
#include "AliEmcalDownscaleFactorsOCDB.h"

#include "TAxisFrame.h"
#include "TNDCLabel.h"
#endif

#include "../../helpers/cdb.C"
#include "../../helpers/math.C"
#include "../../helpers/string.C"

TH1 *extractRejection(const std::string_view filename, const std::string_view dirname, int runnumber){
  auto cdb = AliCDBManager::Instance();
  if(!cdb->IsDefaultStorageSet()) cdb->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", getYearForRunNumber(runnumber))); 
  cdb->SetRun(runnumber);
  PWG::EMCAL::AliEmcalDownscaleFactorsOCDB *downscalehandler = PWG::EMCAL::AliEmcalDownscaleFactorsOCDB::Instance();
  downscalehandler->SetRun(runnumber);
  std::cout << "Reading " << filename << std::endl;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  reader->cd(dirname.data());
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto corrhist = static_cast<TH2 *>(histlist->FindObject("hTriggerCorrelation"));
  auto binMB = corrhist->GetYaxis()->FindBin("MB");
  auto fracMB = corrhist->ProjectionX("fracMB", binMB, binMB);
  fracMB->SetDirectory(nullptr);
  fracMB->Scale(1./fracMB->GetBinContent(binMB));
  // correct for downscaling
  std::map<std::string, std::string> triggers = {{"EMC7", "CEMC7-B-NOPF-CENT"}, {"EG2", "CEMC7EG2-B-NOPF-CENT"}, {"EJ2", "CEMC7EJ2-B-NOPF-CENT"}, 
                                                 {"DMC7", "CDMC7-B-NOPF-CENT"}, {"DG2", "CDMC7DG2-B-NOPF-CENT"}, {"DJ2", "CDMC7DJ2-B-NOPF-CENT"}};
  for(auto t : triggers) {
    auto weight = downscalehandler->GetDownscaleFactorForTriggerClass(t.second.data());
    std::cout << "Found weight " << weight << " for trigger " << t.first << " (" << t.second << ")" << std::endl;
    if(weight) {
      auto b = fracMB->GetXaxis()->FindBin(t.first.data());
      fracMB->SetBinContent(b, fracMB->GetBinContent(b)/weight);
      fracMB->SetBinError(b, fracMB->GetBinError(b)/weight);
    }
  }
  invert(fracMB);
  return fracMB;
}

std::vector<int> getListOfRuns(const std::string_view inputdir, const std::string_view filename){
  std::cout << "Searching input dir " << inputdir << ", file " << filename << std::endl;
  std::string dircontent(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data());
  std::vector<int> result;
  for(const auto &d : tokenize(dircontent)){
    if(!is_number(d)) continue;
    if(gSystem->AccessPathName(Form("%s/%s/%s", inputdir.data(), d.data(), filename.data()))) continue;
    result.emplace_back(std::stoi(d));
  }
  std::sort(result.begin(), result.end(), std::less<int>());
  std::cout << "Found " << result.size() << " runs ..." << std::endl;
  return result;
}

void makeRunByRunTrendRejectionMinBias(const std::string_view filename, const std::string_view inputdir = ".", const std::string_view dirname = "ClusterQA_INT7"){
  std::array<std::string, 8> triggers = {{"EG1", "EG2", "EJ1", "EJ2", "DG1", "DG2", "DJ1", "DJ2"}};
  std::map<std::string, std::pair<double, double>> ranges = {{"EG1", {0., 10000.}}, {"EG2", {0., 1000.}}, {"EJ1", {0., 10000.}}, {"EJ2", {0., 4000.}}, 
                                                             {"DG1", {0., 20000.}}, {"DG2", {0., 2000.}}, {"DJ1", {0., 100000.}}, {"DJ2", {0., 50000.}}};
  std::map<std::string, TGraphErrors *> trendgraphs;
  for(const auto & t : triggers) trendgraphs[t] = new TGraphErrors;
  
  for(auto r : getListOfRuns(inputdir, filename)){
    std::string periodfile = Form("%s/%09d/%s", inputdir.data(), r, filename.data());
    std::cout << "Processing " << periodfile << std::endl;
    std::unique_ptr<TH1> periodhist(extractRejection(periodfile, dirname, r));
    for(const auto &t : triggers){
      auto triggertrend = trendgraphs[t];
      auto triggerbin = periodhist->GetXaxis()->FindBin(t.data());
      auto current = triggertrend->GetN();
      triggertrend->SetPoint(current, r, periodhist->GetBinContent(triggerbin));
      triggertrend->SetPointError(current, 0., periodhist->GetBinError(triggerbin));
    }
  }

  auto plot = new TCanvas("trendingRejection2017", "Trending trigger rejection 2017", 1200, 800);
  plot->Divide(4,2);

  for(auto itrg : ROOT::TSeqI(0, triggers.size())){
    plot->cd(itrg+1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    auto trgrange = ranges[triggers[itrg]];
    auto trgtrend = trendgraphs[triggers[itrg]];
    (new ROOT6tools::TAxisFrame(Form("trendframe%s", triggers[itrg].data()), "run", "Trigger rejection", trgtrend->GetX()[0], trgtrend->GetX()[trgtrend->GetN()-1], trgrange.first, trgrange.second))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.2, 0.9, 0.45, 0.95, Form("Trigger: %s", triggers[itrg].data())))->Draw();
    trgtrend->SetMarkerColor(kBlack);
    trgtrend->SetLineColor(kBlack);
    trgtrend->SetMarkerStyle(20);
    trgtrend->Draw("epsame");
  }
  plot->cd();
  plot->Update();

  std::unique_ptr<TFile> periodwriter(TFile::Open("RejectionTrendingFracMB_runs.root", "RECREATE"));
  for(const auto &t : trendgraphs) t.second->Write(t.first.data());
}