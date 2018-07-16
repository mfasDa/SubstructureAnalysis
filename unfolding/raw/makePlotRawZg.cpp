#ifndef __CLING__
#include <memory>
#include <string>
#include <ROOT/RDataFrame.hxx>
#include <RStringView.h>
#include <TFile.h>

#include <TAxisFrame.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/string.C"

std::string findJetSubstructureTree(const std::string_view inputfile){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  std::string treename;
  for(auto k : *(gDirectory->GetListOfKeys())){
    if(contains(k->GetName(), "jetSubstructure") || contains(k->GetName(), "JetSubstructure")){
      treename = k->GetName();
      break;
    }
  }
  std::cout << "Found tree "<< treename << std::endl;
  return treename;
}

std::string extractTrigger(const std::string_view inputfile){
  std::string result;
  auto stripped  = inputfile.substr(0, inputfile.find(".root"));
  for(auto t : tokenize(std::string(stripped), '_')) {
    if(contains(t, "INT7˜") || contains(t, "EJ") || contains(t, "EG")) {
      result = t;
      break;
    }
  }
  return result;
}

float extractR(const std::string_view inputfile){
  float result;
  auto stripped  = inputfile.substr(0, inputfile.find(".root"));
  for(auto t : tokenize(std::string(stripped), '_')) {
    if(contains(t, "R")) {
      result = std::stof(t.substr(1))/10.;
      break;
    }
  }
  return result;
}

void makePlotRawZg(const std::string_view inputfile, double ptmin, double ptmax){
  ROOT::RDataFrame substructureframe(findJetSubstructureTree(inputfile), inputfile);
  std::vector<double> zgbinning = {0.};
  double current = 0.;
  while(current < 0.5) {
    current += 0.05;
    zgbinning.push_back(current); 
  }
  auto histraw = substructureframe.Filter(Form("PtJetRec > %f && PtJetRec < %f && NEFRec < 0.98", ptmin, ptmax))
                               .Histo1D({Form("rawZgRec_%d_%d", int(ptmin), int(ptmax)), Form("raw zg distribution for %.1f GeV/c < p_{t} < %.1f GeV/c", ptmin, ptmax), int(zgbinning.size())-1, zgbinning.data()}, "ZgMeasured");
  auto hist = new TH1D(*histraw);
  hist->Scale(1./hist->Integral());

  auto plot = new ROOT6tools::TSavableCanvas(Form("rawzg_%d_%d", int(ptmin), int(ptmax)), Form("raw z_{g} for %.1f GeV/c < p_{t} < %.1f GeV/c distribution", ptmin, ptmax), 800, 600);
  plot->cd();

  (new ROOT6tools::TAxisFrame(Form("zgframe_%d_%d", int(ptmin), int(ptmax)), "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.55, 0., 0.4))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.55, 0.84, 0.89, 0.89, Form("pp, #sqrt{s} = 13 TeV, %s", extractTrigger(inputfile).data())))->Draw();
  (new ROOT6tools::TNDCLabel(0.55, 0.78, 0.89, 0.83, Form("Jets, R=%.1f", extractR(inputfile))))->Draw();
  (new ROOT6tools::TNDCLabel(0.45, 0.72, 0.89, 0.77, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptmin, ptmax)))->Draw();
  Style{kRed, 24}.SetStyle<TH1>(*hist);
  hist->Draw("epsame");
  plot->SaveCanvas(plot->GetName());
}