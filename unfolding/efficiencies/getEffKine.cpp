#ifndef __CLING__
#include <memory>
#include <sstream>
#include <vector>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/substructuretree.C"
#include "../binnings/binningZg.C"

std::pair<double, double> getPtRange(std::string_view trigger){
  std::pair<double, double> result;
  if(trigger == "INT7") result = {20., 80.};
  else if(trigger == "EJ2") result = {60., 120.};
  else if(trigger == "EJ1") result = {80., 200.};
  return result;
}

void getEffKine(const std::string_view trigger){
  std::array<Color_t, 10> colors = {{kRed, kBlue, kGreen, kOrange, kTeal, kViolet, kGray, kAzure, kMagenta, kCyan}};
  auto zgbinning = getZgBinningFine(), ptbinning = getPtBinningPart(trigger);
  auto range = getPtRange(trigger);
  std::vector<TH2*> effdata;

  auto plot = new ROOT6tools::TSavableCanvas(Form("effkine_%s", trigger.data()), Form("Kinematic efficiencies for trigger %s", trigger.data()), 1200, 1000);
  plot->Divide(2,2);
  int ipad = 1;
  for(auto r : ROOT::TSeqI(2, 6)){
    std::stringstream fullname, cutname, effname, filename;
    fullname << "FullR" << std::setw(2) << std::setfill('0') << r;
    cutname << "CutR" << std::setw(2) << std::setfill('0') << r;
    effname << "KineEffR" << std::setw(2) << std::setfill('0') << r;
    filename << "JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << r << "_INT7_merged.root"; 
    ROOT::RDataFrame df(GetNameJetSubstructureTree(filename.str().data()), filename.str().data());
    auto full = df.Filter("NEFRec < 0.98").Histo2D({fullname.str().data(), "; z_{g}; p_{t,jet} (GeV/c)", int(zgbinning.size())-1, zgbinning.data(), int(ptbinning.size())-1, ptbinning.data()}, "ZgTrue", "PtJetSim", "PythiaWeight");
    auto cut = df.Filter(Form("NEFRec < 0.98 && PtJetRec > %f && PtJetRec < %f", range.first, range.second)).Histo2D({fullname.str().data(), "; z_{g}; p_{t,jet} (GeV/c)", int(zgbinning.size())-1, zgbinning.data(), int(ptbinning.size())-1, ptbinning.data()}, "ZgTrue", "PtJetSim", "PythiaWeight");

    auto eff = new TH2D(*static_cast<TH2D *>(cut.GetPtr()));
    eff->SetDirectory(nullptr);
    eff->Divide(eff, full.GetPtr(), 1., 1., "b");
    effdata.push_back(eff);

    plot->cd(ipad++);
    (new ROOT6tools::TAxisFrame(Form("Frame_%s_R%02d", trigger.data(), r), "z_{g}", "p_{t}", 0., 0.8, 0., 1.1))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.82, 0.45, 0.89, Form("%s, R=%.1f", trigger.data(), double(r)/10.)))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.6, 0.3, 0.89, 0.89);
    leg->Draw();
    auto igraph = 0;
    for(auto ipt : ROOT::TSeqI(0, eff->GetYaxis()->GetNbins())){
      auto ptcent = eff->GetYaxis()->GetBinCenter(ipt+1);
      if(ptcent < range.first ||  ptcent > range.second) continue;
      auto effbin = eff->ProjectionX(Form("%s_%d", eff->GetName(), ipt+1), ipt+1, ipt+1);
      effbin->SetDirectory(nullptr);
      effbin->SetStats(false);
      Style{colors[igraph], static_cast<Style_t>(igraph + 24)}.SetStyle<TH1>(*effbin);
      igraph++;
      effbin->Draw("epsame");
      leg->AddEntry(effbin, Form("%.1f GeV/c < p_{t,part} < %.1f GeV/c", eff->GetYaxis()->GetBinLowEdge(ipt+1), eff->GetYaxis()->GetBinUpEdge(ipt+1)), "lep");
    }
  }
  plot->cd();
  plot->SaveCanvas(plot->GetName());

  std::stringstream outfilename;
  outfilename << "effKine_" << trigger << ".root";
  std::unique_ptr<TFile> effwriter(TFile::Open(outfilename.str().data(), "RECREATE"));
  effwriter->cd();
  for(auto e : effdata) e->Write();
}