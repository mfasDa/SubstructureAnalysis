#ifndef __CLING__
#include <array>
#include <memory>
#include <string>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TH2.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#endif

#include "../../helpers/graphics.C"

std::array<double, 6> extractQuantiles(TH1 *slice){
  std::array<double, 6> result;
  result[0] = slice->GetMean();
  result[1] = slice->GetMeanError();

  double quantile = 0.5;
  double median;
  slice->GetQuantiles(1, &quantile, &median);
  result[2] = median;
  result[3] = 0;

  result[4] = slice->GetRMS();
  result[5] = slice->GetRMSError();
  return result;
}

std::array<TGraphErrors *, 3> getEnergyScaleForRadius(TFile &reader, double r){
  TGraphErrors *mean = new TGraphErrors, *median = new TGraphErrors, *width = new TGraphErrors;
  reader.cd(Form("EnergyScaleResults_FullJet_R%02d_INT7", int(r*10.)));
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto hdiffRaw = static_cast<THnSparse *>(histlist->FindObject("hPtDiff"));
  // Make NEF cut
  hdiffRaw->GetAxis(1)->SetRangeUser(0., 0.98);
  auto h2d = std::unique_ptr<TH2>(hdiffRaw->Projection(2,0));
  for(auto ib : ROOT::TSeqI(0, h2d->GetXaxis()->GetNbins())){
    auto projected = std::unique_ptr<TH1>(h2d->ProjectionY("py", ib+1, ib+1));
    auto quantiles = extractQuantiles(projected.get());
    mean->SetPoint(ib, h2d->GetXaxis()->GetBinCenter(ib+1), quantiles[0]);
    mean->SetPointError(ib, h2d->GetXaxis()->GetBinWidth(ib+1)/2., quantiles[1]);
    median->SetPoint(ib, h2d->GetXaxis()->GetBinCenter(ib+1), quantiles[2]);
    median->SetPointError(ib, h2d->GetXaxis()->GetBinWidth(ib+1)/2., quantiles[3]);
    width->SetPoint(ib, h2d->GetXaxis()->GetBinCenter(ib+1), quantiles[4]);
    width->SetPointError(ib, h2d->GetXaxis()->GetBinWidth(ib+1)/2., quantiles[6]);
  }
  std::array<TGraphErrors *, 3> result = {{mean, median, width}};
  return result;
}

void extractJetEnergyScale(const std::string_view filename = "AnalysisResults.root"){
  auto plot = new TCanvas("energyscaleplot", "Energy scale", 1200,  600);
  plot->Divide(3,1);

  plot->cd(1);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto meanframe = new TH1F("meanframe", "; p_{t,part} (GeV/c); <(p_{t,det} - p_{t,part})/p_{t,part}>", 200, 0., 200.);
  meanframe->SetDirectory(nullptr);
  meanframe->SetStats(false);
  meanframe->GetYaxis()->SetRangeUser(-1., 1.);
  meanframe->Draw("axis");

  auto leg = new TLegend(0.7, 0.65, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->Draw();

  plot->cd(2);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto medianframe = new TH1F("medianframe", "; p_{t,part} (GeV/c); median((p_{t,det} - p_{t,part})/p_{t,part})", 200, 0., 200.);
  medianframe->SetDirectory(nullptr);
  medianframe->SetStats(false);
  medianframe->GetYaxis()->SetRangeUser(-1., 1.);
  medianframe->Draw("axis");;

  plot->cd(3);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto widthframe = new TH1F("widthframe", "; p_{t,part} (GeV/c); #sigma((p_{t,det} - p_{t,part})/p_{t,part})", 200, 0., 200.);
  widthframe->SetDirectory(nullptr);
  widthframe->SetStats(false);
  widthframe->GetYaxis()->SetRangeUser(0., 1.);
  widthframe->Draw("axis");;

  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  std::map<double, Style> radii = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, {0.4, {kGreen, 26}}, {0.5, {kViolet, 27}}};
  for(const auto r : radii){
    auto enscale = getEnergyScaleForRadius(*reader, r.first);

    for(auto i : ROOT::TSeqI(0,3)){
      plot->cd(i+1);
      r.second.SetStyle<TGraph>(*enscale[i]);
      enscale[i]->Draw("epsame");
      if(i==0) leg->AddEntry(enscale[i], Form("R=%.1f", r.first), "lep");  
    }
  }
  plot->cd();
  plot->Update();
}