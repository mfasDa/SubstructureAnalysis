#ifndef __CLING__
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>

#include "TSavableCanvas.h"
#include "TAxisFrame.h"
#include "TNDCLabel.h"
#include "TDefaultLegend.h"
#endif

#include "../helpers/graphics.C"

TH1 *getEfficiency(const std::string_view filename, const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL){
  std::cout << "Reading " << filename << std::endl;
  try {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::stringstream dirbuilder;
    dirbuilder << "ChargedParticleQA_" << tracktype.data();
    auto dir = static_cast<TDirectoryFile *>(reader->Get(dirbuilder.str().data()));
    auto histlist = static_cast<TList *>(static_cast<TKey *>(dir->GetListOfKeys()->At(0))->ReadObj());
    auto norm = static_cast<TH1 *>(histlist->FindObject(Form("hEventCount%s", trigger.data())));
    auto det = std::unique_ptr<THnSparse>(static_cast<THnSparse *>(histlist->FindObject(Form("hPtEtaPhiAll%s", trigger.data())))),
         part = std::unique_ptr<THnSparse>(static_cast<THnSparse *>(histlist->FindObject("hPtEtaPhiAllTrue")));
    // Look in front of EMCAL
    if(restrictEMCAL){
      det->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      det->GetAxis(2)->SetRangeUser(1.4, 3.1);
      part->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      part->GetAxis(2)->SetRangeUser(1.4, 3.1);
    } else {
      det->GetAxis(1)->SetRangeUser(-0.8, 0.8);
      part->GetAxis(1)->SetRangeUser(-0.8, 0.8);
    }
    auto projectedDet = det->Projection(0);
    std::unique_ptr<TH1> projectedPart(part->Projection(0));
    projectedDet->Sumw2();
    projectedPart->Sumw2();
    projectedDet->SetDirectory(nullptr);
    projectedPart->SetDirectory(nullptr);
    projectedDet->Divide(projectedDet, projectedPart.get(), 1., 1., "b");
    return projectedDet;
  } catch (...) {
    std::cout << "Failure ... " << std::endl;
    return nullptr;
  }
}

void comparePtHardBins(const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL){
  std::vector<int> runs = {272400, 274094, 274653, 275246, 277016, 279312, 280998},
                   pthardbins = {3, 10, 15};

  auto plot = new ROOT6tools::TSavableCanvas(Form("effpthard_%s_%s_%s", tracktype.data(), trigger.data(), restrictEMCAL ? "EMCAL" : "full"), "Run comparison efficiency pthard", 1200, 1000);
  plot->Divide(2, 2);

  std::array<Style, 7> runstyles = {{{kRed, 24}, {kBlue, 25}, {kGreen, 27}, {kOrange, 28}, {kViolet, 29}, {kTeal, 30}, {kGray, 31}}};
  for(auto ipt : ROOT::TSeqI(0, pthardbins.size())){
    plot->cd(ipt+1);
    (new ROOT6tools::TAxisFrame(Form("efframe_%d", ipt), "p_{t} (GeV/c)", "tracking efficiency", 0., 100., 0., 1.1))->Draw("axis");
    std::stringstream labeltext;
    labeltext << "p_{t,h}-bin " << pthardbins[ipt];
    if(!ipt) labeltext << ", " << trigger << ", track type " << tracktype << ", " << (restrictEMCAL ? "EMCAL" : "full") << " acceptance";
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.65, 0.89, labeltext.str().data()))->Draw();
    TLegend *leg = nullptr;
    if(!ipt) {
      leg = new ROOT6tools::TDefaultLegend(0.7, 0.15, 0.89, 0.55);
      leg->Draw();
    }
    for(auto ir : ROOT::TSeqI(0, runs.size())){
      auto hist = getEfficiency(Form("%02d/%d/AnalysisResults.root", pthardbins[ipt], runs[ir]), tracktype, trigger, restrictEMCAL);
      hist->SetName(Form("eff_%d_%d", pthardbins[ipt], runs[ir]));
      runstyles[ir].SetStyle<TH1>(*hist);
      hist->Draw("epsame");
      if(leg) leg->AddEntry(hist, Form("Run %d", runs[ir]), "lep");
    }
  }
  plot->cd();
  plot->SaveCanvas(plot->GetName());
}