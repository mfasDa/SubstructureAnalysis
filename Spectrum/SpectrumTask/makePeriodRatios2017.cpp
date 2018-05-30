#ifndef __CLING__
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#include <TPaveText.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"

std::string findDirectory(const TFile &reader, const std::string_view jettype, const std::string_view trigger){
  std::string result;
  for(auto d : *(gDirectory->GetListOfKeys())){
    std::string_view keyname = d->GetName();
    if(keyname.find(jettype) == std::string::npos) continue;
    if(keyname.find(trigger) == std::string::npos) continue;
    result = std::string(keyname); 
  }
  return result;
}

std::vector<TH1 *> readTriggerHistos(const std::string_view filename, const std::string_view jettype, const std::string_view trigger){
  std::vector<TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  auto dirname = findDirectory(*reader, jettype, trigger);
  if(!dirname.length()) return result;
  reader->cd(dirname.data());
  for(auto k : *(gDirectory->GetListOfKeys())){
    auto h = static_cast<TH1 *>(static_cast<TKey *>(k)->ReadObj()); 
    h->SetDirectory(nullptr);
    result.emplace_back(h);
  }
  return result;
}

void makePeriodRatios2017(const std::string_view jettype, const std::string_view trigger, const std::string_view triggercluster){
  std::map<std::string, Style> periods = {
    {"LHC17h", {kRed, 24}}, {"LHC17i", {kBlue, 25}}, {"LHC17j", {kGreen, 26}}, {"LHC17k", {kViolet, 27}}, {"LHC17l", {kTeal, 28}},
    {"LHC17m", {kMagenta, 29}}, {"LHC17o", {kBlack, 30}} 
  };

  auto plot = new ROOT6tools::TSavableCanvas(Form("periodRstios2017%s%s", jettype.data(), trigger.data()), Form("Period ratios 2017 %s in trigger %s", jettype.data(), trigger.data()), 1200, 1000);
  plot->Divide(2,2);

  int ipad = 1;
  TLegend *leg(nullptr);
  for(auto irad : ROOT::TSeqI(2,6)){
    plot->cd(ipad);
    (new ROOT6tools::TAxisFrame(Form("JetsRatioFrameR%02d%s%s", irad, jettype.data(), trigger.data()), "p_{t,jet} (GeV/c)", "period / sum 2017", 0., 200., 0.5, 1.7))->Draw("axis");

    gPad->SetRightMargin(0.2);

    if(ipad == 1) {
      leg = new ROOT6tools::TDefaultLegend(0.8, 0.5, 0.99, 0.89);
      leg->Draw();

      (new ROOT6tools::TNDCLabel(0.15, 0.91, 0.5, 0.99, Form("%s, trigger %s", jettype.data(), trigger.data())))->Draw();
    }

    (new ROOT6tools::TNDCLabel(0.12, 0.10, 0.22, 0.17, Form("R=%.1f", double(irad)/10.)))->Draw();
    ipad++;
  }

  // Read the
  std::stringstream reffilename;
  reffilename << "LHC17/JetSpectra_" << triggercluster << ".root";
  auto refhists = readTriggerHistos(reffilename.str(), jettype, trigger);

  // Process periods
  for(const auto &p : periods){
    std::stringstream infilename;
    infilename << p.first << "/JetSpectra_" << triggercluster << ".root";
    auto histos = readTriggerHistos(infilename.str(), jettype, trigger);
    
    ipad = 1;
    for(auto irad : ROOT::TSeqI(2, 6)){
      plot->cd(ipad);
      auto hist = *std::find_if(histos.begin(), histos.end(), [irad](const TH1 *hist) -> bool { return std::string(hist->GetName()).find(Form("R%02d", irad)) != std::string::npos; });
      auto ref = *std::find_if(refhists.begin(), refhists.end(), [irad](const TH1 *hist) -> bool { return std::string(hist->GetName()).find(Form("R%02d", irad)) != std::string::npos; });
      hist->Divide(hist, ref, 1., 1., "b");
      p.second.SetStyle<TH1>(*hist);
      hist->Draw("epsame");
      if(ipad == 1) leg->AddEntry(hist, p.first.data(), "lep");
      ipad++;
    }
  }

  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}