#ifndef __CLING__
#include <map>
#include <memory>
#include <string>

#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TSystem.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"

TH1 *getPeriodClusterSpectrum(const std::string_view filename, const std::string_view trigger, const std::string_view detector) {
  TH1 *result(nullptr);
  if(gSystem->AccessPathName(filename.data())) return result;     // file not found
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  reader->cd("ANY");
  for(auto k : *gDirectory->GetListOfKeys()) {
    const std::string_view keyname = k->GetName();
    if((keyname.find(trigger) != std::string::npos) && (keyname.find(detector) != std::string::npos)){
      result = static_cast<TH1 *>(static_cast<TKey *>(k)->ReadObj());
      result->SetDirectory(nullptr);
      break;
    }
  }
  return result;
}

void periodComparison1617(const std::string_view trigger){
  std::map<std::string, Style> styles = {{"LHC16h", {kRed, 24}}, {"LHC16i", {kBlue, 25}}, {"LHC16j", {kGreen, 26}}, {"LHC16k", {kViolet, 27}}, {"LHC16o", {kMagenta, 28}},
                                         {"LHC16p", {kTeal, 29}}, {"LHC17h", {kOrange, 30}}, {"LHC17i", {kGray, 24}}, {"LHC17j", {kRed-5, 25}}, {"LHC17k", {kAzure-4, 26}}, 
                                         {"LHC17l", {kGreen-5, 27}}, {"LHC17m", {kMagenta - 8, 28}}, {"LHC17o", {kCyan +3, 29}}}; 

  std::string detector = (trigger[0] == 'E' ? "EMCAL" : "DCAL");
  std::string triggername = std::string(trigger);
  if(triggername.find("MB") != std::string::npos) triggername = "MB";

  auto plot = new ROOT6tools::TSavableCanvas(Form("ClusterPeriodComparison1617_%s_%s", triggername.data(), detector.data()), Form("Cluster period comparison %s %s 2016/2017", triggername.data(), detector.data()), 800, 600);
  plot->SetLogy();

  double xrange = (triggername == "MB" ? 30.: 100.), yrange = (triggername == "MB" ? : 1e-8, 1e-7);
  (new ROOT6tools::TAxisFrame(Form("clusterAxis_%s_%s", triggername.data(), detector.data()), "E_{cl} (GeV)", "1/N_{trg} dN_{cl}/dE (GeV^{-1})", 0., xrange, yrange, 100.))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.45, 0.55, 0.89, 0.89);
  leg->SetNColumns(2);
  leg->Draw();

  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.35, 0.87, Form("%s, %s", detector.data(), triggername.data())))->Draw();

  for(const auto &t : styles){
    auto spec = getPeriodClusterSpectrum(Form("%s/ClusterSpectra_INT7.root", t.first.data()), triggername, detector);
    if(!spec) continue;
    t.second.SetStyle<TH1>(*spec);
    spec->Draw("epsame");
    leg->AddEntry(spec, t.first.data(), "lep");
  }
  plot->Update();

  plot->SaveCanvas(plot->GetName());
}