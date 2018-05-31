#ifndef __CLING__
#include <algorithm>
#include <array>
#include <iostream>
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

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"

TH1 *ReadJetSpectrum(TFile &reader, double r, const std::string_view jettype, const std::string_view trigger){
  reader.cd(Form("%s_%s", jettype.data(), trigger.data()));
  std::string matchpattern = Form("JetSpectrum_%s_R%02d_%s", jettype.data(), int(r*10.), trigger.data());
  TH1 *hist(nullptr);
  for(auto k : *gDirectory->GetListOfKeys()){
    if(std::string_view(k->GetName()).find(matchpattern) != std::string::npos){
      hist = static_cast<TH1 *>(static_cast<TKey *>(k)->ReadObj());
    }
  }
  hist->SetDirectory(nullptr);
  return hist;
}

std::vector<TH1 *> GetTriggerEfficiencies(TFile &reader, const std::string_view jettype, const std::string_view trigger) {
  std::vector<TH1 *> result;
  for(auto r : ROOT::TSeqI(2, 6)) {
    std::unique_ptr<TH1> mbref(ReadJetSpectrum(reader, float(r)/10., jettype, "INT7"));
    auto spec = ReadJetSpectrum(reader, float(r)/10., jettype, trigger);
    spec->Divide(spec, mbref.get(), 1., 1., "b");
    result.emplace_back(spec);
  }
  return result;  
}

void FillTriggerPanel(TFile &reader, const std::string_view jettype, const std::string_view trigger, bool doLeg) {
  (new ROOT6tools::TAxisFrame(Form("Frame_%s_%s", jettype.data(), trigger.data()), "p_{t,j} (GeV/c)", "Trigger efficiency", 0., 200., 0., 1.2))->Draw("axis");

  TLegend *leg(nullptr);
  if(doLeg){
    leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.4);
    leg->Draw();
  }

  (new ROOT6tools::TNDCLabel(0.65, 0.42, 0.89, 0.47, Form("%s, Trigger %s", jettype.data(), trigger.data())))->Draw();

  auto efficiencies = GetTriggerEfficiencies(reader, jettype, trigger);
  std::map<int, Style> triggers  = {{2, {kRed, 24}}, {3, {kBlue, 25}}, {4, {kGreen, 27}}, {5, {kViolet, 28}}};
  for(const auto &t : triggers){
    auto spec = std::find_if(efficiencies.begin(), efficiencies.end(), [&t](const TH1 * eff) -> bool { return std::string_view(eff->GetName()).find(Form("R%02d", t.first)) != std::string::npos; });
    t.second.SetStyle<TH1>(**spec);
    (*spec)->Draw("epsame");
    if(leg) leg->AddEntry(*spec, Form("R = %.1f", float(t.first)/10.), "lep");
  }
}

ROOT6tools::TSavableCanvas *MakePlotJetType(TFile &reader, const std::string_view jettype){
  auto plot = new ROOT6tools::TSavableCanvas(Form("TriggerEfficiencyComparisonRadius_%s", jettype.data()), Form("Trigger efficiencies %s", jettype.data()), 1200, 1000);
  plot->Divide(2,2);

  std::array<std::string, 4> triggers = {{"EJ1", "EJ2", "EG1", "EG2"}};
  for(auto ipad  : ROOT::TSeqI(0, 4)){
    plot->cd(ipad+1);
    FillTriggerPanel(reader, jettype, triggers[ipad], ipad == 0);
  }
  plot->cd();
  plot->Update();

  return plot;
}

void makeTriggerEfficienciesR(const std::string_view filename = "JetSpectraANY.root"){
  std::array<std::string, 2> jettypes = {{"FullJets", "NeutralJets"}};
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  for(const auto j : jettypes) {
    auto plot =MakePlotJetType(*reader, j);
    plot->SaveCanvas(plot->GetName());
  }
}