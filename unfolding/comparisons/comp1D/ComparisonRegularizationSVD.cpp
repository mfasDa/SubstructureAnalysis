#ifndef __CLING__
#include <array>
#include <map>
#include <memory>
#include <string>
#include <RStringView.h>
#include <TFile.h>

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

struct JetDef {
  std::string fJetType;
  double fJetRadius;
  std::string fTrigger;
};

TH1 *getIterationHist(TFile &reader, int reg){
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

std::map<int, TH1 *> getUnfoldedHists(const std::string_view inputfile){
  std::map<int, TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  // determine number of regulariztions
  int nreg(0);
  for(auto k: *reader->GetListOfKeys()) {
    if(contains(k->GetName(), "regularization")) nreg++;
  }
  for(auto ireg : ROOT::TSeqI(1, nreg+1)){
    if(auto h = getIterationHist(*reader, ireg)) result[ireg] = h;
  }
  return result;
}

std::string getFileTag(const std::string_view infile){
  const char *tagremove = "unfoldedEnergySvd_";
  std::string filetag = basename(infile);
  filetag.erase(filetag.find(tagremove), strlen(tagremove));
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

JetDef getJetType(const std::string_view filetag) {
  auto tokens = tokenize(std::string(filetag), '_');
  return {tokens[0], double(std::stoi(tokens[1].substr(1)))/10., tokens[2]};
}

void ComparisonRegularizationSVD(const std::string_view inputfile){
  auto regularizations = getUnfoldedHists(inputfile);
  auto tag = getFileTag(inputfile);
  auto jd = getJetType(tag);

  auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonRegSvd_%s", tag.data()), "Conparison regularization SVD", 1200, 600);
  plot->Divide(2,1);
  
  plot->cd(1);
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "dN/dp_{t} ((GeV/c)^{-1})", 0., 200., 1e-2, 1e8))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.6, 0.89, 0.89);
  leg->Draw();
  (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.45, 0.22, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();

  plot->cd(2);
  (new ROOT6tools::TAxisFrame("ratframe", "p_{t} (GeV/c)", "ratio to reg = 4", 0., 200., 0., 2.))->Draw("axis");

  std::array<Color_t, 10> colors = {{kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kAzure, kGray, kMagenta, kCyan}};
  std::array<Style_t, 10> markers = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};

  for(auto ireg : ROOT::TSeqI(1, 11)){
    auto hist = regularizations[ireg];
    Style{colors[ireg-1], markers[ireg-1]}.SetStyle<TH1>(*hist);
    plot->cd(1);
    hist->Draw("epsame");
    leg->AddEntry(hist, Form("regularization = %d", ireg), "lep");
    
    if(ireg == 4) continue;
    plot->cd(2);
    auto ratio = histcopy(hist);
    ratio->SetDirectory(nullptr);
    ratio->SetName(Form("Ratio_%d_4", ireg));
    ratio->Divide(regularizations[4]);
    ratio->Draw("epsame");
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}