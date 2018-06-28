#ifndef __CLING__
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"

std::map<std::string, std::vector<TH1 *>> getRatios(const std::string_view jettype, const std::string_view filename){
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));

  std::vector<TH1 *> mbref;
  reader->cd(Form("%s_INT7", jettype.data()));
  for(auto r : ROOT::TSeqI(2, 6)){
    auto hist = static_cast<TH1 *>(gDirectory->Get(Form("RawJetSpectrum_%s_R%02d_INT7_ANY", jettype.data(), r)));
    hist->SetName(Form("INT7_R%02d", r));
    hist->Sumw2();
    mbref.emplace_back(hist);
  }

  std::map<std::string, std::vector<TH1 *>> result;
  const std::array<std::string, 2> emctriggers = {{"EJ1", "EJ2"}};
  for(auto t : emctriggers) {
    std::vector<TH1 *> ratiohists;
    reader->cd(Form("%s_%s", jettype.data(), t.data()));
    for(auto r : ROOT::TSeqI(2, 6)){
      auto trg = static_cast<TH1 *>(gDirectory->Get(Form("RawJetSpectrum_%s_R%02d_%s_ANY", jettype.data(), r, t.data())));
      if(!trg) trg = static_cast<TH1 *>(gDirectory->Get(Form("RawJetSpectrum_%s_R%02d_%ssub_ANY", jettype.data(), r, t.data())));
      trg->SetDirectory(nullptr);
      trg->SetName(Form("%s_R%02d", t.data(), r));
      trg->Sumw2();
      auto myref = *(std::find_if(mbref.begin(), mbref.end(), [&r](const TH1 *h) {std::string_view histname(h->GetName()); return histname.find(Form("R%02d", r)) != std::string::npos; }));
      trg->Divide(trg, myref, 1., 1., "b");
      ratiohists.emplace_back(trg);
    }
    result.insert({t, ratiohists});
  }
  return result;
}

void makeRatioComparisonRaw(const std::string_view jettype, const std::string_view inputfile){
  auto plot = new ROOT6tools::TSavableCanvas(Form("ratiosraw_%s", jettype.data()), Form("Trigger efficiency for jet type %s", jettype.data()), 1200, 600); 
  plot->Divide(2,1);

  int ipad(1);
  std::map<int, Style> rstyles = {{2, {kRed, 24}}, {3, {kBlue, 25}}, {4, {kGreen, 26}}, {5, {kViolet, 27}}};
  for(auto trigger : getRatios(jettype, inputfile)){
    plot->cd(ipad);

    (new ROOT6tools::TAxisFrame(Form("effframe_%s_%s", jettype.data(), trigger.first.data()), "p_{t,jet} (GeV/c)", "Trigger efficiency", 0., 100, 0., 1.1))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.83, 0.5, 0.89, Form("%s, %s", jettype.data(), trigger.first.data())))->Draw();

    TLegend *leg(nullptr);
    if(ipad == 1) {
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.4);
      leg->Draw();
    }

    for(auto r : ROOT::TSeqI(2, 6)){
      auto hist = *std::find_if(trigger.second.begin(), trigger.second.end(), [&r](const TH1 *h) { std::string_view hn(h->GetName()); return hn.find(Form("R%02d", r)) != std::string::npos; });
      rstyles[r].SetStyle<TH1>(*hist);
      hist->Draw("epsame");
      if(leg) leg->AddEntry(hist, Form("R=%.1f", double(r)/10.), "lep");
    }
    ipad++;
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}