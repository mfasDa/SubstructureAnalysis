#ifndef __CLING__
#include <map>
#include <memory>
#include <set>
#include <RStringView.h>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH2.h>
#include <THnSparse.h>
#include <TList.h>
#include <TProfile.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"

THnSparse *getTrackTHnSparse(const std::string_view inputfile){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  auto histlist = static_cast<TList *>(reader->Get("AliEmcalTrackingQATask_histos"));
  auto hsparse = static_cast<THnSparse *>(histlist->FindObject("fTracks"));
  return hsparse;
}

TGraphErrors *getPtResolution(THnSparse *hs, int tracktype) {
  int ttbin = hs->GetAxis(4)->FindBin(tracktype);
  hs->GetAxis(4)->SetRange(ttbin, ttbin);
  std::unique_ptr<TH2> projected(hs->Projection(5,0));

  if(projected->GetEntries()){
    std::unique_ptr<TProfile> prof(projected->ProfileX());
    auto result = new TGraphErrors();
    int np(0);
    for(auto p : ROOT::TSeqI(0, prof->GetXaxis()->GetNbins())){
      result->SetPoint(np, prof->GetXaxis()->GetBinCenter(p+1), prof->GetBinContent(p+1));
      result->SetPointError(np, prof->GetXaxis()->GetBinWidth(p+1)/2., prof->GetBinError(p+1));
      np++;
    }
    return result;
  } else {
    return nullptr;
  }
}

void makePtResolution(const std::string_view inputfile){
  std::map<int, TGraphErrors *> ptres;
  auto hsparse = getTrackTHnSparse(inputfile);
  
  for(auto tt : ROOT::TSeqI(0, 8)) {
    auto resgraph = getPtResolution(hsparse, tt);
    if(resgraph) ptres[tt] = resgraph;
  }

  auto plot = new ROOT6tools::TSavableCanvas("ptresplotPWGJEQA", "Pt resolution comparison", 800, 600);
  plot->cd();

  (new ROOT6tools::TAxisFrame("ptrepframe", "p_{t} (GeV/c)", "#sigma(p_{t})/p_{t}", 0., 100., 0., 0.3))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.5, 0.5, 0.89);
  leg->Draw();
  (new ROOT6tools::TNDCLabel(0.35, 0.1, 0.89, 0.15, "pp, #sqrt{s} = 13 TeV, hybrid tracks 2018beta"))->Draw();

  std::map<int, std::string> tttitles = {{0, "Track type I (wTRD 4 tls)"}, {1, "Track type IIa (wTRD 4 tls)"}, {2, "Track type IIb (wTRD 4 tls)"},
                                         {3, "Track type III (wTRD 4 tls)"}, {4, "Track type I (woTRD 4 tls)"}, {5, "Track type IIa (woTRD 4 tls)"},
                                         {6, "Track type IIb (woTRD 4 tls)"}, {7, "Track type III (woTRD 4 tls)"}};

  std::map<int, Style> ttstyles = {{0, {kRed, 24}}, {1, {kBlue, 25}}, {2, {kGreen, 26}}, {3, {kViolet,  26}}, {4, {kOrange, 27}}, {5, {kGray, 28}},
                                   {6, {kMagenta, 29}}, {9, {kCyan, 30}}};
  for(auto tt : ptres) {
    ttstyles[tt.first].SetStyle<TGraphErrors>(*(tt.second));
    tt.second->Draw("epsame");
    leg->AddEntry(tt.second, tttitles[tt.first].data(), "lep");
  }
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}