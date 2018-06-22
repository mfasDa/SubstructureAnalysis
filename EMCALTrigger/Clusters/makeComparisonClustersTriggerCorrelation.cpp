#ifndef __CLING__
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <TFile.h>
#include <TH1.h>
#include <TLine.h>
#include <TBox.h>
#include <RStringView.h>

#include <TAxisFrame.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

std::array<std::string, 5> emcaltriggers = {{"EMC7", "EG1", "EG2", "EJ1", "EJ2"}};

std::vector<TH1 *> readClusterTurnon(){
  std::vector<TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open("ClusterTurnon.root"));
  for(auto t : emcaltriggers){
    auto hist = static_cast<TH1 *>(reader->Get(Form("Turnon%s", t.data())));
    hist->SetDirectory(nullptr);
    result.emplace_back(hist);
  }
  return result;
}

TH1 *readRejection(){
  std::unique_ptr<TFile> reader(TFile::Open("RejectionFracMB.root"));
  auto hist = static_cast<TH1 *>(reader->Get("rejection"));
  hist->SetDirectory(nullptr);
  return hist;
}

void makeComparisonClustersTriggerCorrelation(){
  auto plot = new ROOT6tools::TSavableCanvas("ComparisonTriggerRejectionMethod", "Comparison Trigger Rejection various methods", 1400, 800.);
  plot->Divide(5,2);
  auto turnons  = readClusterTurnon();
  auto rejection = readRejection();

  std::map<std::string, double> ranges = {{"EMC7", 150.}, {"EG1", 10000.}, {"EG2", 800.}, {"EJ1", 10000.}, {"EJ2", 4000.}};

  int icase = 1;
  for(auto t : emcaltriggers){
    plot->cd(icase);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.06);
    (new ROOT6tools::TAxisFrame(Form("TurnonFrame%s", t.data()), "E_{cl} (GeV)", "Trigger rejection", 0., 100., 0., ranges[t]))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.45, 0.15, 0.92, 0.22, Form("Trigger: %s", t.data())))->Draw();
    auto hist = *std::find_if(turnons.begin(), turnons.end(), [&t](const TH1 *hist) {std::string_view histname(hist->GetName()); return histname.find(t) != std::string::npos; });
    hist->Draw("epsame");

    auto rejectionbin = rejection->GetXaxis()->FindBin(t.data());
    auto rv = rejection->GetBinContent(rejectionbin), re = rejection->GetBinError(rejectionbin);

    auto center = new TLine(1., rv, 99, rv);
    center->SetLineColor(kBlack);
    center->Draw("lsame");

    auto error = new TBox(1., rv - re, 99, rv + re);
    error->SetLineWidth(0);
    error->SetFillStyle(3002);
    error->Draw("boxsame");

    plot->cd(icase + 5);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.06);
    (new ROOT6tools::TAxisFrame(Form("RatioFrame%s", t.data()), "E_{cl} (GeV)", "Turnon / Rejection", 0., 100., 0.5, 1.5))->Draw("axis");
    auto ratio = static_cast<TH1 *>(hist->Clone(Form("Ratio%s", t.data())));
    ratio->SetDirectory(nullptr);
    ratio->Scale(1./rv);
    ratio->Draw("epsame");

    icase++;
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}