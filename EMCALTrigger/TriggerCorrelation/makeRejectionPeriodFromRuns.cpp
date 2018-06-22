#ifndef __CLING__
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>
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
#include "../../helpers/string.C"


TH1 *getCorrectedRunCorrelation(int run, const std::string_view basename, const std::string_view basedir){
  auto cdb = AliCDBManager::Instance();
  if(!cdb->IsDefaultStorageSet()) cdb->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", getYearForRunNumber(run))); 
  cdb->SetRun(run);
  PWG::EMCAL::AliEmcalDownscaleFactorsOCDB *downscalehandler = PWG::EMCAL::AliEmcalDownscaleFactorsOCDB::Instance();
  downscalehandler->SetRun(run);
  std::stringstream filename;
  filename << basedir << "/" << std::setw(9) << std::setfill('0') << run << "/" << basename;
  std::unique_ptr<TFile> reader(TFile::Open(filename.str().data(), "READ"));
  auto corrhist = static_cast<TH2 *>(reader->Get("hTriggerCorrelation"));
  auto binMB = corrhist->GetYaxis()->FindBin("MB");
  auto fracMB = corrhist->ProjectionX("fracMB", binMB, binMB);
  fracMB->SetDirectory(nullptr);
  for(auto b : ROOT::TSeqI(0, fracMB->GetXaxis()->GetNbins())){
    std::string_view triggerlabel(fracMB->GetXaxis()->GetBinLabel(b+1));
    if(triggerlabel.length())
      std::cout << "Run " << run << ", Raw fraction " << triggerlabel << ": " << fracMB->GetBinContent(b+1) << std::endl;
  }
  // correct for downscaling
  // L0 triggers have identical downscale factors as MB - not correcting
  std::map<std::string, std::string> triggers = {{"EG2", "CEMC7EG2-B-NOPF-CENT"}, {"EJ2", "CEMC7EJ2-B-NOPF-CENT"}, 
                                                 {"DG2", "CDMC7DG2-B-NOPF-CENT"}, {"DJ2", "CDMC7DJ2-B-NOPF-CENT"}};
  auto refweight = downscalehandler->GetDownscaleFactorForTriggerClass("CINT7-B-NOPF-CENT");
  for(auto t : triggers) {
    auto weight = downscalehandler->GetDownscaleFactorForTriggerClass(t.second.data());
    std::cout << "Found weight " << weight << " for trigger " << t.first << " (" << t.second << ")" << std::endl;
    if(weight == refweight) {
      std::cout << "Don't apply weigth because trigger was downscaled synchronized with MB" << std::endl;
      continue;
    }
    if(weight) {
      auto b = fracMB->GetXaxis()->FindBin(t.first.data());
      std::cout << "Corrected weight fir trigger :" << (fracMB->GetBinContent(b)/weight) << " +- " << (fracMB->GetBinError(b)/weight) << std::endl;
      fracMB->SetBinContent(b, fracMB->GetBinContent(b)/weight);
      fracMB->SetBinError(b, fracMB->GetBinError(b)/weight);
    }
  }
  return fracMB;
}

std::vector<int> getListOfRuns(const std::string_view basedir){
  std::vector<int> runs;  
  for(auto d : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", basedir.data())).Data())){
    if(is_number(d)) runs.emplace_back(std::stoi(d));
  }
  return runs;
}

TH1 *extractRejection(const std::string_view basefile, const std::string_view basedir){
  TH1 *rejection = nullptr;
  for(auto r : getListOfRuns(basedir)){
    auto raw = getCorrectedRunCorrelation(r, basefile, basedir);
    if(!rejection) rejection = raw;
    else {
      rejection->Add(raw);
      delete raw;
    }
  }
  auto binMB = rejection->GetXaxis()->FindBin("MB");
  rejection->Scale(1./rejection->GetBinContent(binMB));
  invert(rejection);
  return rejection;
}

void makeRejectionPeriodFromRuns(const std::string_view basename = "TriggerCorrelation_INT7.root", const std::string_view basedir = "."){
  auto plot = new TCanvas("plotRejection", "Rejection factors", 800, 600);
  plot->cd();
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto fracMB = extractRejection(basename, basedir);
  std::vector<std::string_view> triggers;
  for(auto b : ROOT::TSeqI(0, fracMB->GetXaxis()->GetNbins())){
    std::string_view binlabel(fracMB->GetXaxis()->GetBinLabel(b+1));
    if(binlabel.length()) triggers.emplace_back(binlabel);
  }
  fracMB->SetTitle("");
  fracMB->SetStats(false);
  fracMB->GetXaxis()->SetRange(1, triggers.size());
  fracMB->GetYaxis()->SetTitle("Rejection factor");
  fracMB->SetMarkerColor(kBlack);
  fracMB->SetLineColor(kBlack);
  fracMB->SetMarkerStyle(24);
  fracMB->Draw("eptext0");

  // Print extracted rejection values:
  for(auto b : ROOT::TSeqI(fracMB->GetXaxis()->GetNbins())){
    std::string_view binlabel(fracMB->GetXaxis()->GetBinLabel(b+1));
    if(binlabel.length()) std::cout << binlabel << ": " << fracMB->GetBinContent(b+1) << " +- "  << fracMB->GetBinError(b+1) << std::endl; 
  }

  // Write out histogram
  std::unique_ptr<TFile> writer(TFile::Open("RejectionFracMB.root", "RECREATE"));
  writer->cd();
  fracMB->SetName("rejection");
  fracMB->SetTitle("Rejection factors");
  fracMB->Write();
}