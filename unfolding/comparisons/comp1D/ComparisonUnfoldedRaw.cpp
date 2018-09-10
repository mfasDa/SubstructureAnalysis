#ifndef __CLING__
#include <map>
#include <memory>
#include <string>

#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"
#include "../../../helpers/root.C"
#include "../../../helpers/string.C"

TH1 *getRegularizationHist(TFile &reader, int reg){
  TH1 *result = nullptr;
  std::string regname = Form("regularization%d", reg);
  if(reader.GetListOfKeys()->FindObject(regname.data())){
    reader.cd(regname.data());
    std::string histname = Form("unfoldedReg%d", reg);
    result = static_cast<TH1 *>(gDirectory->Get(histname.data()));
    if(result){
      result->SetDirectory(nullptr);
      normalizeBinWidth(result);
    }
  }
  return result;
}

TH1 *getiIterationHist(TFile &reader, int reg){
  TH1 *result = nullptr;
  std::string regname = Form("iteration%d", reg);
  if(reader.GetListOfKeys()->FindObject(regname.data())){
    reader.cd(regname.data());
    std::string histname = Form("unfolded_iter%d", reg);
    result = static_cast<TH1 *>(gDirectory->Get(histname.data()));
    if(result){
      result->SetDirectory(nullptr);
      normalizeBinWidth(result);
    }
  }
  return result;
}

std::map<int, TH1 *> getUnfoldedHists(const std::string_view inputfile){
  bool isSVD = contains(inputfile, "SVD");
  std::map<int, TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  // determine number of regulariztions
  if(isSVD){
    int nreg(0);
    for(auto k: *reader->GetListOfKeys()) {
      if(contains(k->GetName(), "regularization")) nreg++;
    }
    for(auto ireg : ROOT::TSeqI(1, nreg+1)){
      if(auto h = getRegularizationHist(*reader, ireg)) result[ireg] = h;
    }
  } else {
    for(auto it : ROOT::TSeqI(1, 36)){
      result[it] = getiIterationHist(*reader, it);
    }
  }
  return result;
}

TH1 *getRaw(const std::string_view inputfile){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  reader->cd("rawlevel");
  auto hist = static_cast<TH1 *>(gDirectory->Get("hraw"));
  hist->SetDirectory(nullptr);
  normalizeBinWidth(hist);
  return hist;
}

std::string getFileTag(const std::string_view infile){
  std::string filetag = basename(infile);
  const std::string tagremove = contains(filetag, "SVD") ? "corrected1DSVD_" : "corrected1DBayes_";
  filetag.erase(filetag.find(tagremove), tagremove.length());
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

float getRadius(const std::string_view filetag) {
  return float(std::stoi(std::string(filetag.substr(1))))/10.;
}

void ComparisonUnfoldedRaw(const std::string_view filename){
  auto unfolded = getUnfoldedHists(filename);
  auto hraw = getRaw(filename);
  auto tag = getFileTag(filename);
  auto radius = getRadius(tag);
  bool isSVD = contains(filename, "SVD");

  auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonUnfoldedRaw%s_%s", (isSVD ? "SVD" : "Bayes"), tag.data()), "Conparison unfolded raw", 800, 600);
  plot->cd();
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "dN/dp_{t} ((GeV/c)^{-1})", 0., 250., 1e-10, 10))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.6, 0.89, 0.89);
  leg->Draw();
  (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.45, 0.22, Form("jets, R=%.1f", radius)))->Draw();

  std::array<Color_t, 10> colors = {{kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kAzure, kGray, kMagenta, kCyan}};
  std::array<Style_t, 10> markers = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};

  Style{kBlack, 20}.SetStyle<TH1>(*hraw);
  plot->cd(1);
  hraw->Draw("epsame");
  leg->AddEntry(hraw, "Raw", "lep");

  for(auto ireg : ROOT::TSeqI(1, 11)){
    auto hist = unfolded[ireg];
    Style{colors[ireg-1], markers[ireg-1]}.SetStyle<TH1>(*hist);
    plot->cd(1);
    hist->Draw("epsame");
    leg->AddEntry(hist, Form("%s = %d", (isSVD ? "regularization" : "iteration"), ireg), "lep");
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}