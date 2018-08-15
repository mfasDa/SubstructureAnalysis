#ifndef __CLING__
#include <memory>
#include <string>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TSavableCanvas.h>
#endif

#include "../helpers/filesystem.C"
#include "../helpers/graphics.C"
#include "../helpers/math.C"
#include "../helpers/root.C"

TH1 *readIterOne(const std::string_view fullname){
  std::unique_ptr<TFile> reader(TFile::Open(fullname.data(), "READ"));
  reader->cd("iteration1");
  auto hist = static_cast<TH1 *>(gDirectory->Get("unfolded_iter1"));
  hist->SetDirectory(nullptr);
  normalizeBinWidth(hist);
  return hist;  
}

std::string getFileTag(const std::string_view infile){
  const char *tagremove = "unfoldedEnergyBayes_";
  std::string filetag = basename(infile);
  filetag.erase(filetag.find(tagremove), strlen(tagremove));
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

void comparePriors(const std::string_view priorfile){
  auto withpriors = readIterOne(Form("unfolded1D_withPriors/%s", priorfile.data())),
       withoutpriors = readIterOne(Form("unfolded1D_withoutPriors/%s", priorfile.data()));

  auto plot = new ROOT6tools::TSavableCanvas(Form("priorcomp_%s", getFileTag(priorfile).data()), "Prior comparison", 1200, 600);
  plot->Divide(2,1);
  plot->cd(1);
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "dN/dpt", 0., 400, 1e-3, 1e7))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.75, 0.89, 0.89);
  leg->Draw();
  Style{kBlack, 24}.SetStyle<TH1>(*withpriors);
  withpriors->Draw("epsame");
  leg->AddEntry(withpriors, "With priors", "lep");
  Style{kRed, 25}.SetStyle<TH1>(*withoutpriors);
  withoutpriors->Draw("epsame");
  leg->AddEntry(withoutpriors, "With priors", "lep");

  plot->cd(2);
  (new ROOT6tools::TAxisFrame("ratioframe", "p_{t} (GeV/c)", "without/with priors", 0., 400., 0., 1.))->Draw("axis");
  auto ratio = histcopy(withoutpriors);
  ratio->SetDirectory(nullptr);
  ratio->Divide(withpriors);
  Style{kBlack, 20}.SetStyle<TH1>(*ratio);
  ratio->Draw("epsame");
  plot->cd();
  plot->SaveCanvas(plot->GetName());
}