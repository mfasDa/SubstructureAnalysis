#ifndef __CLING__
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

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

struct JetDef {
  std::string fJetType;
  double fJetRadius;
  std::string fTrigger;
};

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
  bool isSVD = contains(inputfile, "Svd");
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

std::string getFileTag(const std::string_view infile){
  std::string filetag = basename(infile);
  const std::string tagremove = contains(filetag, "Svd") ? "unfoldedEnergySvd_" : "unfoldedEnergyBayes_";
  filetag.erase(filetag.find(tagremove),tagremove.length());
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

JetDef getJetType(const std::string_view filetag) {
  auto tokens = tokenize(std::string(filetag), '_');
  return {tokens[0], double(std::stoi(tokens[1].substr(1)))/10., tokens[2]};
}

void ComparisonBayesSVD(const std::string_view bayesfile, const std::string_view svdfile){
  auto unfoldedBayes = getUnfoldedHists(bayesfile), unfoldedSvd = getUnfoldedHists(svdfile);
  auto tag = getFileTag(bayesfile);
  auto jd = getJetType(tag);
  auto plot = new ROOT6tools::TSavableCanvas(Form("ComparisonBayesSvd_%s", tag.data()), "Comparison Bayes-SVD", 1200, 600);
  plot->Divide(2,1);

  plot->cd(1);
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "dN/dp_{t} (GeV/c)", 0., 200., 1e-1, 1e7))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.45, 0.89, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.75, 0.89, 0.89);
  leg->Draw();

  const int kDefaultIterBayes = 4, kDefaultRegSvd =  4;
  auto specBayes = unfoldedBayes[kDefaultIterBayes], specSvd = unfoldedSvd[kDefaultRegSvd];
  Style{kRed, 24}.SetStyle<TH1>(*specBayes);
  Style{kBlue, 25}.SetStyle<TH1>(*specSvd);
  specBayes->Draw("epsame");
  specSvd->Draw("epsame");
  leg->AddEntry(specBayes, Form("Bayes, niter = %d", kDefaultIterBayes), "lep");
  leg->AddEntry(specSvd, Form("SVD, reg = %d", kDefaultRegSvd), "lep");

  plot->cd(2);
  (new ROOT6tools::TAxisFrame("ratioframe", "p_{t} (GeV/c)", "Bayes/SVD", 0., 200., 0., 2.))->Draw("axis"); 
  auto ratio = histcopy(specBayes);
  ratio->SetName("RatioBayesSvd");
  ratio->Divide(specSvd);
  Style{kBlack, 20}.SetStyle<TH1>(*ratio);
  ratio->Draw("epsame");

  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}