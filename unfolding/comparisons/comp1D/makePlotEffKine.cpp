#ifndef __CLING__ 
#include <RStringView.h>

#include <TFile.h>
#include <TH1.h>

#include "TAxisFrame.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/string.C"

struct JetDef {
  std::string fJetType;
  double fJetRadius;
  std::string fTrigger;
};

std::string getFileTag(const std::string_view infile){
  std::string filetag = basename(infile);
  const std::string tagremove = contains(filetag, "Svd") ? "unfoldedEnergySvd_" : "unfoldedEnergyBayes_";
  filetag.erase(filetag.find(tagremove), tagremove.length());
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

JetDef getJetType(const std::string_view filetag) {
  auto tokens = tokenize(std::string(filetag), '_');
  return {tokens[0], double(std::stoi(tokens[1].substr(1)))/10., tokens[2]};
}

void makePlotEffKine(const std::string_view inputfile){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  auto effhist = static_cast<TH1 *>(reader->Get("effKine"));
  effhist->SetDirectory(nullptr);

  auto tag = getFileTag(inputfile);
  auto jd = getJetType(tag);
  auto plot = new ROOT6tools::TSavableCanvas(Form("effKine_%s", tag.data()), "Kinematic efficiency plot", 800, 600);
  plot->cd();
  (new ROOT6tools::TAxisFrame("effframe", "p_{t}", " kinematic efficiency", 0., 200., 0., 1.))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.45, 0.15, 0.89, 0.22, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
  Style{kBlack, 20}.SetStyle<TH1>(*effhist);
  effhist->Draw("epsame");
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}
