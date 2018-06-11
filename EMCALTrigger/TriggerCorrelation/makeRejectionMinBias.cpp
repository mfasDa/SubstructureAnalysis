#ifndef __CLING__
#include <iostream>
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
  PWG::EMCAL::AliEmcalDownscaleFactorsOCDB *downscalehandler = PWG::EMCAL::AliEmcalDownscaleFactorsOCDB::Instance();
  downscalehandler->SetRun(runnumber);
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

void makeRejectionMinBias(const std::string_view filename, int runnumber, const std::string_view directory = "ClusterQA_INT7"){
  auto cdb = AliCDBManager::Instance();
  cdb->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", getYearForRunNumber(runnumber))); 
  cdb->SetRun(runnumber);
  auto plot = new TCanvas("plotRejection", "Rejection factors", 800, 600);
  plot->cd();
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto fracMB = extractRejection(filename, directory, runnumber);
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
}