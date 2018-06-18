#ifndef __CLING__
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <RStringView.h>
#include <TFile.h>
#include <TF1.h>
#include <TGraphErrors.h>
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
  gDirectory->ls();
  std::vector<TObject *> listdata;
  for(auto o : *(gDirectory->GetListOfKeys())) listdata.emplace_back(static_cast<TKey *>(o)->ReadObj());
  for(auto r : kJetRadii) {
    auto spec = static_cast<TH1 *>(*std::find_if(listdata.begin(), listdata.end(), [r](const TObject *o) -> bool { 
      const std::string_view histname = o->GetName(); 
      return (histname.find(Form("R%02d", int(r*10.))) != std::string::npos) && (histname.find("JetSpectrum") == 0); 
    }));
    spec->SetDirectory(nullptr);
    spec->Rebin(4);
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

std::map<double, std::pair<double, double>> fillTriggerPanel(const triggerdata &data) {
  std::map<std::string, double> yrange = {{"EG1", 10000.}, {"EG2", 1000.}, {"EJ1", 10000.}, {"EJ2", 4000.}};
  std::map<double, Style> kStyles = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, {0.4, {kGreen, 26}}, {0.5, {kViolet, 27}}};
  std::map<double, std::pair<double, double>> rejectionfit;
  (new ROOT6tools::TAxisFrame(Form("rejectionframe_%s", data.fName.data()), "p_{t} (GeV/c)", "Rejection factor", 0., 100., 0., yrange[data.fName]))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.35, 0.89, Form("Trigger: %s", data.fName.data())))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.5, 0.15, 0.89, 0.4);
  leg->Draw();
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
    rejectionfit[r] = {fit->GetParameter(0), fit->GetParError(0)};
  }
  gPad->Update();
  return rejectionfit;
}

TGraphErrors *makeTrendGraph(std::map<double, std::pair<double, double>> &data){
  TGraphErrors *trendgraph = new TGraphErrors;
  std::set<double> jetradii;
  for(auto r : data) jetradii.insert(r.first);
  int npoints(0);
  for(auto r : jetradii) {
    trendgraph->SetPoint(npoints, r, data[r].first);
    trendgraph->SetPointError(npoints, 0., data[r].second);
    npoints++;
  }
  return trendgraph;
}

void makeTriggerRejection(const std::string_view jettype, const std::string_view inputfile){
  std::map<std::string, Style> styles = {{"EG1", {kRed, 24}}, {"EG2", {kViolet, 25}}, {"EJ1", {kBlue, 26}}, {"EJ2", {kGreen, 27}}};
  auto plot = new ROOT6tools::TSavableCanvas(Form("Rejection%s", jettype.data()), Form("Rejection for %s", jettype.data()), 1200, 1000);
  plot->Divide(2,2);
  auto rejectionRadius = new ROOT6tools::TSavableCanvas(Form("RejectionTrendRadius%s", jettype.data()), Form("Rejection trending vs radius for %s", jettype.data()), 800, 600);
  rejectionRadius->cd();
  rejectionRadius->SetLeftMargin(0.14);
  rejectionRadius->SetRightMargin(0.06);
  (new ROOT6tools::TAxisFrame(Form("axisTrendR%s", jettype.data()), "R", "Rejection", 0., 0.7, 0., 10000.))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.79, 0.89, 0.89);
  leg->SetNColumns(4);
  leg->Draw();
  std::map<std::string, TGraphErrors *> trendings;
  int ipad = 1;
  for(const auto &t : getTriggerRejections(jettype, inputfile)) {
    plot->cd(ipad++);
    auto fits = fillTriggerPanel(t);
    rejectionRadius->cd();
    auto triggertrend = makeTrendGraph(fits);
    styles[t.fName].SetStyle<TGraphErrors>(*triggertrend);
    triggertrend->Draw("epsame");
    leg->AddEntry(triggertrend, t.fName.data(), "lep");
    trendings[t.fName] = triggertrend;
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());

  rejectionRadius->cd();
  rejectionRadius->Update();
  rejectionRadius->SaveCanvas(rejectionRadius->GetName());

  std::unique_ptr<TFile> trendfile(TFile::Open(Form("rejectionTrendR%s.root", jettype.data()), "RECREATE"));
  trendfile->cd();
  for(auto t : trendings) t.second->Write(t.first.data());
}