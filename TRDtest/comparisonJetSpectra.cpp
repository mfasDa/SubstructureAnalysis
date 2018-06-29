#ifndef __CLING__
#include <memory>
#include <vector>
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

#include "../helpers/graphics.C"
#include "../helpers/string.C"

std::vector<TH1 *> getSpectra(const std::string_view filename, const std::string_view jettype, const std::string_view trigger) {
  std::vector<TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  reader->cd(Form("%s_%s", jettype.data(), trigger.data()));
  std::vector<TH1 *> histlist;
  for(auto k : TRangeDynCast<TKey>(gDirectory->GetListOfKeys()))
    histlist.emplace_back(k->ReadObject<TH1>());
  for(auto r : ROOT::TSeqI(2, 6)){
    std::string rstring = Form("R%02d", r);
    auto finder = [&](const TH1 *hist){
      std::string_view histname(hist->GetName());
      std::vector<std::string> match = {"JetSpectrum", std::string(jettype), std::string(trigger), rstring},
                               tokens = tokenize(std::string(histname), '_');
      bool found(true);
      for(auto m : match) {
        if(std::find(tokens.begin(), tokens.end(), m) == tokens.end()) {
          found = false;
          break;
        }
      }
      return found;
    };
    auto hist = *std::find_if(histlist.begin(), histlist.end(), finder);
    hist->SetDirectory(nullptr);
    result.emplace_back(hist);
  }
  return result;
}

TH1 *makeRatio(const TH1 *withTRD, const TH1 *withoutTRD){
  TH1 *result = static_cast<TH1 *>(withTRD->Clone(Form("RatioWithWithout_%s", withTRD->GetName())));
  result->SetDirectory(nullptr);
  result->Divide(withoutTRD);
  return result;
}

void comparisonJetSpectra(const std::string_view jettype, const std::string_view trigger = "INT7"){
  auto spectrawith = getSpectra("wTRD/JetSpectra_CENT.root", jettype, trigger),
       spectrawithout = getSpectra("woTRD/JetSpectra_CENT.root", jettype, trigger);
  auto plot = new ROOT6tools::TSavableCanvas(Form("TRDcomparison%s", jettype.data()), Form("Comparison w/wo TRD %s", jettype.data()), 1600, 1000);
  plot->Divide(4,2);

  Style withstyle = {kRed, 24}, withoutstyle = {kBlue, 25}, ratiostyle = {kBlack, 20};
  
  int ipad = 1;
  for(auto radius : ROOT::TSeqI(2, 6)) {
    plot->cd(ipad);
    gPad->SetLogy();
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    (new ROOT6tools::TAxisFrame(Form("specframe_%s_R%02d", jettype.data(), radius), "p_{t,jet} (GeV/c)", "1/N_{trg} dN/dp_{t} (GeV/c)", 0., 100., 1e-8, 100))->Draw("axis");
    TLegend *leg(nullptr);
    if(ipad == 1) {
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.75, 0.94, 0.89);
      leg->Draw();
      (new ROOT6tools::TNDCLabel(0.2, 0.22, 0.45, 0.27, jettype.data()))->Draw();
    }
    (new ROOT6tools::TNDCLabel(0.2, 0.15, 0.35, 0.2, Form("R=%.1f", double(radius)/10.)))->Draw();
    
    auto histfinder = [&](const TH1 *hist) { return std::string_view(hist->GetName()).find(Form("R%02d", radius)) != std::string::npos; };
    auto specwith = *std::find_if(spectrawith.begin(), spectrawith.end(), histfinder),
         specwithout = *std::find_if(spectrawithout.begin(), spectrawithout.end(), histfinder);
    withstyle.SetStyle<TH1>(*specwith);
    withoutstyle.SetStyle<TH1>(*specwithout);
    specwith->Draw("epsame");
    specwithout->Draw("epsame");
    if(leg){
      leg->AddEntry(specwithout, "pass1", "lep");
      leg->AddEntry(specwith, "TRDtest", "lep");
    }

    plot->cd(ipad+4);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    (new ROOT6tools::TAxisFrame(Form("Ratioframe_%s_R%02d", jettype.data(), radius), "p_{t,jet} (GeV/c)", "TRDtest / pass1", 0., 100., 0.5, 1.5))->Draw("axis");
    auto ratio = makeRatio(specwith, specwithout);
    ratiostyle.SetStyle<TH1>(*ratio);
    ratio->Draw("epsame");

    ipad++;
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}