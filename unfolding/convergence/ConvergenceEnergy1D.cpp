#ifndef __CLING__
#include <memory>
#include <map>
#include <ROOT/TSeq.hxx>
#include "RStringView.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TH1.h"
#include "TPaveText.h"
#include "TNDCLabel.h"
#endif

void ConvergenceEnergy1D(const std::string_view inputfile) {
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  std::map<int, TH1 *> trends;
  TH1 *pttemplate(nullptr);
  for(auto iter : ROOT::TSeqI(1, 36)){
    reader->cd(Form("iteration%d", iter));
    auto inputhist = static_cast<TH1 *>(gDirectory->Get(Form("unfolded_iter%d", iter)));
    if(iter == 1) {
      pttemplate = inputhist;
      for(auto ptbin : ROOT::TSeqI(0, inputhist->GetXaxis()->GetNbins())){
        auto trendhist = new TH1D(Form("trendPt%d", ptbin), "; iterations; dN/dp_{t}", 41, -0.5, 40.5);
        trends[ptbin+1] = trendhist;
        trendhist->SetDirectory(nullptr);
        trendhist->SetStats(false);
        auto binid = trendhist->GetXaxis()->FindBin(iter);
        trendhist->SetBinContent(binid, inputhist->GetBinContent(ptbin+1));
        trendhist->SetBinError(binid, inputhist->GetBinError(ptbin+1));
      }
    } else {
      for(auto ptbin : ROOT::TSeqI(0, inputhist->GetXaxis()->GetNbins())){
        auto trendhist = trends[ptbin+1];
        auto binid = trendhist->GetXaxis()->FindBin(iter);
        trendhist->SetBinContent(binid, inputhist->GetBinContent(ptbin+1));
        trendhist->SetBinError(binid, inputhist->GetBinError(ptbin+1));
      }
    }
  }

  auto plot = new TCanvas("ConvergenceEnergy", "Convergence Energy", 1200, 1000);
  plot->DivideSquare(trends.size());
  for(auto trendhist : trends) {
    plot->cd(trendhist.first);
    auto prim = trendhist.second;
    prim->SetMarkerColor(kBlack);
    prim->SetLineColor(kBlack);
    prim->SetMarkerStyle(20);
    prim->Draw("ep");

    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.89, 0.3, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", pttemplate->GetXaxis()->GetBinLowEdge(trendhist.first), pttemplate->GetXaxis()->GetBinUpEdge(trendhist.first))))->Draw();
  }
}