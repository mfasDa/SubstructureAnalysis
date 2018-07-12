#ifndef __CLING__
#include <iostream>
#include <sstream>
#include <RStringView.h>
#include <TFile.h>
#include <TDirectoryFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>

#include <TAxisFrame.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
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
      det->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      det->GetAxis(2)->SetRangeUser(1.4, 3.1);
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

void extractTrackingEff(const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL, const std::string_view inputfile = "AnalysisResults.root"){
  auto effhist = getEfficiency(inputfile, tracktype, trigger, restrictEMCAL);
  if(!effhist){
    std::cerr << "Error reading efficiency histogram" << std::endl;
    return;
  }
  
  auto acctype = restrictEMCAL ? "EMCAL" : "Full";
  auto plot = new ROOT6tools::TSavableCanvas(Form("trackingEff_%s_%s_%s", tracktype.data(), trigger.data(), acctype), Form("Tracking efficiency %s, %s, %s acceptance", tracktype.data(), trigger.data(), acctype), 800, 600);
  plot->cd();
  auto frame = new ROOT6tools::TAxisFrame(Form("effFrame_%s_%s_%s", tracktype.data(), trigger.data(), acctype), "p_{t, part} (GeV/c)", "Tracking efficiency x acceptance", 0., 100., 0., 1.1);
  frame->Draw("axis");
  Style{kBlack, 20}.SetStyle<decltype(*effhist)>(*effhist);
  effhist->Draw("epsame");
  (new ROOT6tools::TNDCLabel(0.15, 0.82, 0.55, 0.89, Form("Track type %s, %s, %s acceptance", tracktype.data(), trigger.data(), acctype)))->Draw();
  plot->SaveCanvas(plot->GetName());
 }