#ifndef __CLING__
#include <algorithm>
#include <array>
#include <memory>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#include <TObjString.h>
#include <TPaveText.h>
#endif

#include "../../helpers/graphics.C"

void compareTriggerClusters(const std::string_view trigger, const std::string_view filename = "ClusterSpectra.root"){
  std::map<const std::string, Style> kClusterStyles = {{"ANY", {kBlack, 20}}, {"CENT", {kRed, 24}}, {"CENTNOTRD", {kBlue, 25}}, 
                                                       {"CENTBOTH", {kGreen, 26}}, {"ONLYCENT", {kOrange, 27}}, {"ONLYCENTNOTRD", {kViolet, 28}}};

  auto plot = new TCanvas("comaprisonTriggerClusters", "Comparison trigger clusters", 800, 600);
  plot->SetLogy();
  auto frame = new TH1F("frame", "; E_{cluster} (GeV); 1/N_{trg} dN/dE_{cl} ((GeV)^{-1})", 100, 0., 100.);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(1e-6, 100);
  frame->Draw("axis");

  auto leg = new TLegend(0.7, 0.65, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->Draw();

  auto label = new TPaveText(0.15, 0.15, 0.25, 0.22, "NDC");
  InitWidget<TPaveText>(*label);
  label->AddText(trigger.data());
  label->Draw();

  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  for(auto s : kClusterStyles){
    const std::string &tcl = s.first;
    reader->cd(s.first.data()); 
    TH1 *spechist = nullptr; 
    for(auto h : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
      if(std::string_view(h->GetName()).find(trigger) != std::string::npos){
        spechist = static_cast<TH1 *>(h->ReadObj());
      }
    } 
    spechist->SetDirectory(nullptr);
    s.second.SetStyle<TH1>(*spechist);
    spechist->Draw("epsame"); 
    leg->AddEntry(spechist, s.first.data(), "lep");
  }
  plot->Update();
}