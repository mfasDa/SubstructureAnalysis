#ifndef __CLING__
#include <algorithm>
#include <array>
#include <map>
#include <sstream>
#include <vector>

#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>

#include "TAxisFrame.h"
#include "TNDCLabel.h"
#include "TDefaultLegend.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/string.C"

struct Specname
{
  std::string tag;
  std::string detector;
  std::string trigger;
  std::string cluster;
};

Specname DecodeSpectrumName(const std::string_view specname)
{
  auto tokens = tokenize(std::string(specname), '_');
  return {tokens[0], tokens[1], tokens[2], tokens[3]};
}

std::vector<TH1 *> getClusterSpectraForPeriod(const std::string_view filename, const std::string_view period) {
  std::vector<TH1 *> result;
  std::unique_ptr<TFile> preader(TFile::Open(filename.data(), "READ"));
  preader->cd("ANY");
  for(auto k : *(gDirectory->GetListOfKeys())){
    auto trg = DecodeSpectrumName(k->GetName());
    auto spec = static_cast<TH1 *>(static_cast<TKey *>(k)->ReadObj());
    spec->SetDirectory(nullptr);

    std::stringstream histname;
    if(trg.trigger == "MB") {
      histname << (trg.detector == "EMCAL" ? "E" : "D") << "MB";
    } else histname << trg.trigger;
    histname << "_" << period;
    spec->SetName(histname.str().data());
    result.emplace_back(spec);
  }
  return result;
}

ROOT6tools::TSavableCanvas *PlotDetector(const std::string_view detector, std::map<std::string, std::vector<TH1 *>> &data){
  const std::vector<std::string> triggers = {"MB", "G1", "G2", "J1", "J2"};
  const std::map<std::string, Style> kStyles = {
    {"LHC17h", {kRed, 24}}, {"LHC17i", {kBlue, 25}}, {"LHC17j", {kGreen, 26}}, {"LHC17k", {kViolet, 27}}, {"LHC17l", {kTeal, 28}},
    {"LHC17m", {kMagenta, 29}}, {"LHC17o", {kBlack, 30}} 
  };
  auto plot = new ROOT6tools::TSavableCanvas(Form("ClusterRatio2017_%s", detector.data()), Form("Cluster spectra for %s", detector.data()), 1200, 700);
  plot->Divide(3,2);

  auto reference = data.find("LHC17")->second;

  auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.5, 0.89);
  for(auto i : ROOT::TSeqI(0, triggers.size())){
    std::string trigger = (detector == std::string_view("EMCAL") ? "E" : "D") + triggers[i];
    plot->cd(i+1);
    auto frame = new ROOT6tools::TAxisFrame(Form("frame%s%s", detector.data(), triggers[i].data()), "E_{cl} (GeV)", "Period / sum 2017", 0., 100., 0.5, 1.5);
    frame->SetDirectory(nullptr);
    frame->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.45, 0.22, Form("Trigger: %s", trigger.data())))->Draw();

    for(auto d : data) {
      auto period = d.first;
      if(period == std::string("LHC17")) continue;
      auto spec = *std::find_if(d.second.begin(), d.second.end(), [&trigger, &period] (const TH1 *hist) -> bool {
        return TString(hist->GetName()) == TString(trigger + "_" + period);
      }); 
      auto refspec = *std::find_if(reference.begin(), reference.end(), [&trigger] (const TH1 *hist) -> bool {
        return TString(hist->GetName()) == TString(trigger + "_LHC17");
      }); 
      spec->Divide(spec, refspec, 1., 1., "b");
      kStyles.find(d.first)->second.SetStyle<TH1>(*spec);
      spec->Draw("epsame");
      if(!i) leg->AddEntry(spec, d.first.data(), "lep");
    }
    gPad->Update();
  }

  plot->cd(6);
  leg->Draw();

  plot->cd();
  plot->Update();
  return plot;
}

void makeClusterRatiosPeriods2017(){
  const std::vector<std::string> periods = {"LHC17h", "LHC17i", "LHC17j", "LHC17k", "LHC17l", "LHC17m", "LHC17o", "LHC17"};
  std::map<std::string, std::vector<TH1 *>> data;
  for(const auto &p : periods) data[p] = getClusterSpectraForPeriod(Form("%s/ClusterSpectra_INT7.root", p.data()), p);
  std::array<std::string, 2> detectors = {{"EMCAL", "DCAL"}};
  for(auto d : detectors) {
    auto plot = PlotDetector(d, data);
    plot->SaveCanvas(plot->GetName());
  }
}