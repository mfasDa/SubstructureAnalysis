#ifndef __CLING__
#include <iostream>
#include <map>
#include <memory>
#include <ROOT/TSeq.h>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>

#include "AliCDBManager.h"
#include "AliEmcalDownscaleFactorsOCDB.h"
#endif

#include "../../helpers/cdb.C"
#include "../../helpers/math.C"

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

TH1 *makeTrendHisto(const std::string_view trigger, const std::map<std::string, int> &periods){
  auto hist = new TH1D(Form("trending%s", trigger.data()), Form("Trigger: %s", trigger.data()), periods.size(), -0.5, periods.size() - 0.5);
  hist->SetDirectory(nullptr);
  std::vector<std::string_view> periodnames;
  for(const auto &p : periods) periodnames.push_back(p.first);
  std::sort(periodnames.begin(), periodnames.end(), std::less<std::string_view>());
  for(auto i : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())) hist->GetXaxis()->SetBinLabel(i+1, periodnames[i].data());
  hist->GetYaxis()->SetTitle("Trigger rejection");
  return hist;
}

void makePeriodTrendRejectionMinBias(const std::string_view filename, const std::string_view dirname = "ClusterQA_INT7"){
  std::map<std::string, int> periods = {{"LHC17h", 272610}, {"LHC17i", 274442}, {"LHC17j", 274671}, {"LHC17k", 276508}, 
                                        {"LHC17l", 278216}, {"LHC17m", 280140}, {"LHC17o", 281961}}; // run number needed for downscale correction
  std::array<std::string, 8> triggers = {{"EG1", "EG2", "EJ1", "EJ2", "DG1", "DG2", "DJ1", "DJ2"}};
  std::map<std::string, std::pair<double, double>> ranges = {{"EG1", {0., 10000.}}, {"EG2", {0., 1000.}}, {"EJ1", {0., 10000.}}, {"EJ2", {0., 4000.}}, 
                                                             {"DG1", {0., 20000.}}, {"DG2", {0., 2000.}}, {"DJ1", {0., 100000.}}, {"DJ2", {0., 50000.}}};
  std::map<std::string, TH1 *> trendhists;
  for(const auto & t : triggers) trendhists[t] = makeTrendHisto(t, periods);
  
  for(const auto &p : periods){
    std::string periodfile = Form("%s/%s", p.first.data(), filename.data());
    std::cout << "Processing " << periodfile << std::endl;
    std::unique_ptr<TH1> periodhist(extractRejection(periodfile, dirname, p.second));
    for(const auto &t : triggers){
      auto triggertrend = trendhists[t];
      auto triggerbin = periodhist->GetXaxis()->FindBin(t.data()),
           periodbin = triggertrend->GetXaxis()->FindBin(p.first.data());
      triggertrend->SetBinContent(periodbin, periodhist->GetBinContent(triggerbin));
      triggertrend->SetBinError(periodbin, periodhist->GetBinError(triggerbin));
    }
  }

  auto plot = new TCanvas("trendingRejection2017", "Trending trigger rejection 2017", 1200, 800);
  plot->Divide(4,2);

  for(auto itrg : ROOT::TSeqI(0, triggers.size())){
    plot->cd(itrg+1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    auto trghist = trendhists[triggers[itrg]];
    auto trgrange = ranges[triggers[itrg]];
    trghist->SetStats(false);
    trghist->SetMarkerColor(kBlack);
    trghist->SetLineColor(kBlack);
    trghist->SetMarkerStyle(20);
    trghist->GetYaxis()->SetRangeUser(trgrange.first, trgrange.second);
    trghist->Draw("ep");
  }
  plot->cd();
  plot->Update();

  std::unique_ptr<TFile> periodwriter(TFile::Open("RejectionTrendingFracMB_periods.root", "RECREATE"));
  for(const auto &t : trendhists) t.second->Write();
}