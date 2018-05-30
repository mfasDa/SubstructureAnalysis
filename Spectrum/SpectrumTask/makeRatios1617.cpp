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

void makeRatios1617(const std::string_view jettype, const std::string_view trigger, const std::string_view triggercluster){
  std::map<std::string, Style> periods = {
    {"LHC16i", {kBlue, 25}}, {"LHC16j", {kGreen, 26}}, {"LHC16k", {kViolet, 27}}
  };

  auto plot = new ROOT6tools::TSavableCanvas(Form("periodRatios%s%s", jettype.data(), trigger.data()), Form("Period comparison %s in trigger %s", jettype.data(), trigger.data()), 1200, 1000);
  plot->Divide(2,2);

  int ipad = 1;
  TLegend *leg(nullptr);
  for(auto irad : ROOT::TSeqI(2,6)){
    plot->cd(ipad);
    gPad->SetRightMargin(0.2);
    //(new ROOT6tools::TAxisFrame(Form("JetSpecFrameR%02d%s%s", irad, jettype.data(), trigger.data()), "p_{t,jet} (GeV/c)", "period / sum 2017", 0., 200., 0.5, 2.5))->Draw("axis");
    (new ROOT6tools::TAxisFrame(Form("JetSpecFrameR%02d%s%s", irad, jettype.data(), trigger.data()), "p_{t,jet} (GeV/c)", "period / sum 2017", 0., 200., 0., 800.))->Draw("axis");

    if(ipad == 1) {
      leg = new ROOT6tools::TDefaultLegend(0.8, 0.45, 0.99, 0.89);
      leg->Draw();

      (new ROOT6tools::TNDCLabel(0.15, 0.9, 0.5, 0.99, Form("%s, trigger %s", jettype.data(), trigger.data())))->Draw();
    }

    (new ROOT6tools::TNDCLabel(0.12, 0.1, 0.22, 0.17, Form("R=%.1f", double(irad)/10.)))->Draw();
    ipad++;
  }

  // Read the
  std::stringstream reffilename;
  reffilename << "LHC17/JetSpectra_" << triggercluster << ".root";
  auto refhists = readTriggerHistos(reffilename.str(), jettype, trigger);

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