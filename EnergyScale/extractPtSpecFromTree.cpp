#ifndef __CLING__
#include <memory>
#include <ROOT/RDataFrame.hxx>
#include "RStringView.h" 
#include <TCanvas.h>
#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>

#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../helpers/graphics.C"
#include "../helpers/math.C"
#include "../helpers/string.C"

struct unfoldconfig {
  std::string fJetType;
  double fR;
  std::string fTrigger;
};

unfoldconfig extractFileTokens(const std::string_view filename){
  auto tokens = tokenize(std::string(filename.substr(0, filename.find(".root"))), '_');
  return {tokens[1], double(std::stoi(tokens[2].substr(1,2)))/10., tokens[3]};
}

std::vector<double> makeJetPtBinning(){
  std::vector<double> result = {0.};
  double currentmax = 0.;
  while(currentmax < 10.) {
    currentmax += 1.;
    result.push_back(currentmax);
  }
  while(currentmax < 40.) {
    currentmax += 2.;
    result.push_back(currentmax);
  }
  while(currentmax < 100.) {
    currentmax += 5.;
    result.push_back(currentmax);
  }
  while(currentmax < 300.) {
    currentmax += 10.;
    result.push_back(currentmax);
  }
  while(currentmax < 600.){
    currentmax += 20.;
    result.push_back(currentmax);
  }
  return result;
}

void extractPtSpecFromTree(const std::string_view inputfile){
  auto conf = extractFileTokens(inputfile);
  ROOT::EnableImplicitMT();
  ROOT::RDataFrame substructureframe("jetSubstructure",inputfile.data());
  auto jetptbinning = makeJetPtBinning();
  
  auto spectrumSim = substructureframe.Filter([](double nef) { return nef > 0.02 && nef < 0.98; }, {"NEFSim"}).Histo1D({"ptsim", "jet spectrum pt sim", static_cast<int>(jetptbinning.size())-1, jetptbinning.data()}, "PtJetSim", "PythiaWeight");
  auto spectrumRec = substructureframe.Filter([](double nef) { return nef > 0.02 && nef < 0.98; }, {"NEFRec"}).Histo1D({"ptsim", "jet spectrum pt rec", static_cast<int>(jetptbinning.size())-1, jetptbinning.data()}, "PtJetRec", "PythiaWeight");
  auto ptcorr = substructureframe.Filter([](double nef) { return nef > 0.02 && nef < 0.98; } , {"NEFRec"}).Histo2D({"ptcorr", "pt correlation", static_cast<int>(jetptbinning.size())-1, jetptbinning.data(), static_cast<int>(jetptbinning.size())-1, jetptbinning.data()}, "PtJetSim", "PtJetRec");
  
  //auto spectrumSim = substructureframe.Histo1D({"ptsim", "jet spectrum pt sim", static_cast<int>(jetptbinning.size())-1, jetptbinning.data()}, "PtJetSim", "PythiaWeight");
  //auto spectrumRec = substructureframe.Histo1D({"ptsim", "jet spectrum pt rec", static_cast<int>(jetptbinning.size())-1, jetptbinning.data()}, "PtJetRec", "PythiaWeight");
  //auto ptcorr = substructureframe.Histo2D({"ptcorr", "pt correlation", static_cast<int>(jetptbinning.size())-1, jetptbinning.data(), static_cast<int>(jetptbinning.size())-1, jetptbinning.data()}, "PtJetSim", "PtJetRec");
  //auto spectrum = substructureframe.Histo1D("PtJetSim");//{"ptsim", "jet spectrum pt sim", 300, 0., 300}

  auto plot = new ROOT6tools::TSavableCanvas(Form("specplot_%s_R%02d_%s", conf.fJetType.data(), int(conf.fR * 10.), conf.fTrigger.data()), "Jet spectrum spectrum", 1200, 600);
  plot->Divide(2,1);
  plot->cd(1);
  gPad->SetLogy();
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto frame = new TH1F("specframe", "; p_{t,jet} (GeV/c); d#sigma/dp_{t,jet} (mb/(GeV/c))", 600, 0., 600.);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(1e-12, 100);
  frame->Draw("axis");
  auto resultsim = new TH1D(*spectrumSim);
  auto resultrec = new TH1D(*spectrumRec);
  resultsim->SetDirectory(nullptr);
  resultrec->SetDirectory(nullptr);
  normalizeBinWidth(resultsim);
  Style{kRed, 24}.SetStyle<TH1>(*resultsim);
  resultsim->Draw("epsame");
  normalizeBinWidth(resultrec);
  Style{kBlue, 24}.SetStyle<TH1>(*resultrec);
  resultrec->Draw("epsame");
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
  leg->AddEntry(resultsim, "Particle level", "lep");
  leg->AddEntry(resultrec, "Detector level", "lep");
  leg->Draw();
  (new ROOT6tools::TNDCLabel(0.15, 0.11, 0.5, 0.17, Form("%s, R=%.1f, %s", conf.fJetType.data(), conf.fR, conf.fTrigger.data())))->Draw();

  plot->cd(2);
  gPad->SetLogz();
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.14);
  auto corrhist = new TH2D(*ptcorr);
  corrhist->SetDirectory(nullptr);
  corrhist->SetTitle("");
  corrhist->SetStats(false);
  corrhist->GetXaxis()->SetTitle("p_{t,part} (GeV/c)");
  corrhist->GetYaxis()->SetTitle("p_{t,det} (GeV/c)");
  corrhist->Draw("colz");

  plot->cd();
  plot->SaveCanvas(plot->GetName());
}