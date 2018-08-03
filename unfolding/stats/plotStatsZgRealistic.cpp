#ifndef __CLING__
#include <map>
#include <vector>
#include <sstream>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/substructuretree.C"
#include "../binnings/binningZg.C"

void print(std::vector<double> &vals) {
  auto current = vals.begin();
  std::cout << *current;
  current++;
  while(current != vals.end()){
    std::cout << ",  " << *current;
    current++;
  }
  std::cout << std::endl;
}

void plotStatsZgRealistic(const std::string_view trigger){
  std::vector<double> ptbinning = getPtBinningRealistic(trigger), zgbinning = getZgBinningFine();
  print(ptbinning);
  print(zgbinning);
  std::stringstream canvasname, canvastitle;
  canvasname << "StatZgRealistic" << trigger;
  canvastitle << "Statistics Zg for trigger " << trigger << " with realistic binning";
  auto plot = new ROOT6tools::TSavableCanvas(canvasname.str().data(), canvastitle.str().data(), 1200, 1000);
  plot->Divide(2,2);
  int ipad(1);
  for(auto r : ROOT::TSeqI(2, 6)) {
    std::stringstream filename;
    filename << "JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << r << "_" << trigger << ".root";
    ROOT::RDataFrame df(GetNameJetSubstructureTree(filename.str()), filename.str());
    std::stringstream histname;
    histname << "CountsZgPtRealisticR" << std::setw(2) << std::setfill('0') << r;
    auto hist = df.Filter("NEFRec < 0.98").Histo2D({histname.str().data(), "; z_{g}; p_{t, jet} (GeV/c)", static_cast<int>(zgbinning.size())-1, zgbinning.data(), static_cast<int>(ptbinning.size())-1, ptbinning.data()}, "ZgMeasured", "PtJetRec");
    hist->SetDirectory(nullptr);
    hist->SetStats(false);
    plot->cd(ipad++);
    hist->DrawCopy("TEXT");
    (new ROOT6tools::TNDCLabel(0.25, 0.9, 0.75, 0.96, Form("%s, R = %.1f", trigger.data(), double(r)/10.)))->Draw();
  }
  plot->cd();
  plot->SaveCanvas(plot->GetName());
}