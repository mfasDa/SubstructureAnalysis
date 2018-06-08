#ifndef __CLING__
#include <map>
#include <memory>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TList.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/math.C"

std::vector<TH1 *> getSupermoduleSpectra(const std::string_view trigger, const std::string_view filename){
  std::map<int, Style> smstyles = {{0, {kRed, 24}}, {1, {kBlue, 25}}, {2, {kGreen, 26}}, {3, {kMagenta, 27}}, {4, {kTeal, 28}}, 
                                   {5, {kViolet, 29}}, {6, {kGray, 30}}, {7, {kAzure, 31}}, {8, {kOrange, 32}}, {9, {kBlack, 33}}, 
                                   {10, {kYellow+2, 34}}, {11, {kCyan+2}}};
  std::vector<TH1 *> result;
  auto tcl = 0;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  reader->cd("ClusterQA_Default");
  auto histos = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto norm = static_cast<TH1 *>(histos->FindObject(Form("hTrgClustCounter%s", trigger.data())));
  auto clusters = static_cast<THnSparse *>(histos->FindObject(Form("hClusterTHnSparseAll%s", trigger.data())));
  clusters->GetAxis(5)->SetRange(tcl+1, tcl+1);
  int minsm = trigger[0] == 'E' ? 0 : 12, maxsm = trigger[0] == 'E' ? 11 : 19;
  for(auto sm : ROOT::TSeqI(minsm, maxsm+1)){
    clusters->GetAxis(0)->SetRange(sm+1, sm+1);
    auto spec = clusters->Projection(1);
    spec->SetDirectory(nullptr);
    spec->SetName(Form("%s_SM%d", trigger.data(), sm));
    spec->Scale(1./norm->GetBinContent(tcl+1));
    normalizeBinWidth(spec);
    smstyles[sm].SetStyle<TH1>(*spec);
    result.emplace_back(spec); 
  }
  return result;
}

void comparisonTriggerSupermodules(const std::string_view trigger, const std::string_view filename = "AnalysisResults.root"){
  auto plot = new ROOT6tools::TSavableCanvas(Form("clusterSpecSM%s", trigger.data()), Form("Cluster spectra for different supermodules for trigger %s", trigger.data()), 800, 600);
  plot->cd();
  plot->SetLogy();

  (new ROOT6tools::TAxisFrame(Form("Frame%s", trigger.data()), "E_{cl} (GeV)", "1/N_{trg} dN/dE_{cl} (GeV^{-1})", 0., 100., 1e-7, 100))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.29, 0.22, Form("Trigger: %s", trigger.data())))->Draw();

  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.45, 0.89, 0.89);
  leg->Draw();

  for(auto spec : getSupermoduleSpectra(trigger, filename)){
    auto histname = std::string_view(spec->GetName());
    auto smstring = histname.substr(histname.find("_"+1));
    spec->Draw("epsame");
    leg->AddEntry(spec, smstring.data(), "lep");
  }
  plot->Update();
}