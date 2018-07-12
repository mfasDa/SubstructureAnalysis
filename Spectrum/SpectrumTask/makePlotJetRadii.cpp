#ifndef __CLING__
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"

void makePlotJetRadii(const std::string_view jettype, const std::string_view trigger, const std::string_view filename = "JetSpectra_ANY.root"){
  auto plot = new ROOT6tools::TSavableCanvas(Form("radiuscomparison_%s_%s", jettype.data(), trigger.data()), Form("Radius comparison %s %s", jettype.data(), trigger.data()), 800, 600);
  plot->cd();
  plot->SetLogy();
  (new ROOT6tools::TAxisFrame(Form("axis_%s_%s", jettype.data(), trigger.data()), "p_{t,jet} (GeV/c)", "d#sigma/dp_{t} (mb/(GeV/c))", 0., 200., 1e-11, 100))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.55, 0.22, Form("%s, %s", jettype.data(), trigger.data())))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.55, 0.65, 0.89, 0.89);
  leg->Draw();
  {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd(Form("%s_%s", jettype.data(), trigger.data()));
    auto histos = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
    std::map<int, Style> styles = {{2, {kRed, 24}}, {3, {kBlue, 25}}, {4, {kGreen, 26}}, {5, {kViolet, 27}}};

    for(auto r : ROOT::TSeqI(2, 6)){
      auto spec = std::find_if(histos.begin(), histos.end(), [r](const TKey * h) { std::string_view histname(h->GetName()); return histname.find("JetSpectrum") == 0 && contains(histname, Form("R%02d", r)); } );
      if(spec == histos.end()) continue;
      auto spechist = (*spec)->ReadObject<TH1>();
      spechist->SetDirectory(nullptr);
      styles[r].SetStyle<TH1>(*spechist);
      spechist->Draw("epsame");
      leg->AddEntry(spechist, Form("R=%.1f", float(r)/10.), "lep");
    }
  }
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}