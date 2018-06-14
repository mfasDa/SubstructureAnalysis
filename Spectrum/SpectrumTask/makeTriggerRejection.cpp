#ifndef __CLING__
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <RStringView.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1.h>
#include <TKey.h>
#include <TList.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"

const std::array<double, 4> kJetRadii = {{0.2, 0.3, 0.4, 0.5}}; 
const std::array<std::string, 4> kEMCALtriggers = {{"EJ1", "EJ2", "EG1", "EG2"}};

struct triggerdata {
  std::string               fName;
  std::map<double, TH1 *>   fData; 
};

triggerdata readSpectrumData(TFile &reader, const std::string_view jettype, const std::string_view trigger) {
  triggerdata result;
  result.fName = std::string(trigger);
  reader.cd(Form("%s_%s", jettype.data(), trigger.data()));
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  for(auto r : kJetRadii) {
    auto spec = static_cast<TH1 *>(*std::find_if(histlist->begin(), histlist->end(), [r](const TObject *o) -> bool { 
      const std::string_view histname = o->GetName(); 
      return (histname.find(Form("R%02d", int(r*10.))) != std::string::npos) && (histname.find("JetSpectrum") == 0); 
    }));
    spec->SetDirectory(nullptr);
    result.fData[r] = spec;
  }
  return result;
}

std::vector<triggerdata> getTriggerRejections(const std::string_view jettype, const std::string_view inputfile){
  std::vector<triggerdata> result; 
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  auto mbdata = readSpectrumData(*reader, jettype, "INT7");
  for(const auto &t : kEMCALtriggers){
    auto trgdata = readSpectrumData(*reader, jettype, t);
    for(auto r : kJetRadii){
      trgdata.fData[r]->Divide(mbdata.fData[r]);
    }
    result.emplace_back(trgdata);
  }
  return result;
} 

void fillTriggerPanel(const triggerdata &data) {
  std::map<std::string, double> yrange = {{"EG1", 10000.}, {"EG2", 1000.}, {"EJ1", 10000.}, {"EJ2", 4000.}};
  std::map<double, Style> kStyles = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, {0.4, {kGreen, 26}}, {0.5, {kViolet, 27}}};
  (new ROOT6tools::TAxisFrame(Form("rejectionframe_%s", data.fName.data()), "p_{t} (GeV/c)", "Rejection factor", 0., 100., 0., yrange[data.fName]))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.35, 0.89, Form("Trigger: %s", data.fName.data())))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.5, 0.15, 0.89, 0.4);
  for(auto r : kJetRadii) {
    auto spec = data.fData.find(r)->second;
    kStyles[r].SetStyle<TH1>(*spec);
    spec->Draw("epsame");
    auto fit = new TF1(Form("fit%sR%02d", data.fName.data(), int(r*10.)), "pol0", 0., 100.);
    fit->SetLineColor(kStyles[r].color);
    fit->SetLineStyle(2);
    spec->Fit(fit, "N", "", 20., 50.);
    fit->Draw("lsame");
    leg->AddEntry(spec, Form("R=%.1f: %.1f #pm %.1f", r, fit->GetParameter(0), fit->GetParError(0)), "lep");
  }
  gPad->Update();
}

void makeTriggerRejection(const std::string_view jettype, const std::string_view inputfile){
  auto plot = new ROOT6tools::TSavableCanvas(Form("Rejection%s", jettype.data()), Form("Rejection for %s", jettype.data()), 1200, 1000);
  plot->Divide(2,2);
  int ipad = 1;
  for(const auto &t : getTriggerRejections(jettype, inputfile)) {
    plot->cd(ipad++);
    fillTriggerPanel(t);
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}