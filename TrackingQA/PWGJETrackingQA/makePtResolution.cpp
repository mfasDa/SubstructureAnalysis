#ifndef __CLING__
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
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
#include "../../struct/GraphicsPad.cxx"

struct cut {
  std::string                     fDimension;
  std::pair<double, double>       fRange; 
};

THnSparse *getTrackTHnSparse(const std::string_view inputfile, const std::string_view trigger){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  std::stringstream listname;
  listname << "AliEmcalTrackingQATask";
  if(trigger.length()){
    listname << "_" << trigger;
  }
  listname << "_histos";
  auto histlist = static_cast<TList *>(reader->Get(listname.str().data()));
  auto hsparse = static_cast<THnSparse *>(histlist->FindObject("fTracks"));
  return hsparse;
}

TGraphErrors *getPtResolution(THnSparse *hs, int tracktype, const std::vector<cut> &cuts) {
  int ttbin = hs->GetAxis(4)->FindBin(tracktype);
  hs->GetAxis(4)->SetRange(ttbin, ttbin);
  std::vector<int> cutdimensions;
  cutdimensions.emplace_back(4);
  for(auto c : cuts){
    int axis = -1;
    if(c.fDimension == "eta") axis = 1;
    else if(c.fDimension == "phi") axis = 2;
    hs->GetAxis(axis)->SetRangeUser(c.fRange.first, c.fRange.second);
    std::cout << "Applying cut " << c.fDimension << ": [" << c.fRange.first << "," << c.fRange.second << "]" << std::endl;
    cutdimensions.emplace_back(axis);
  }
  std::unique_ptr<TH2> projected(hs->Projection(5,0));
  for(auto d : cutdimensions) hs->GetAxis(d)->UnZoom();

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

void makePtResolution(const std::string_view inputfile, const std::string_view trigger = ""){
  std::map<int, TGraphErrors *> ptres;
  auto hsparse = getTrackTHnSparse(inputfile, trigger);

  std::vector<cut> cuts;
  if(trigger.length()  && (trigger[0] == 'E' || trigger[0] == 'D')){
    // If EMCAL / DCAL triggers apply eta / phi cuts
    cuts.push_back({"eta", {-0.5, 0.5}});
    if(trigger[0] == 'E') {
      cuts.push_back({"phi", {1.4, 3.1}});
    } else {
      cuts.push_back({"phi", {4.2, 5.4}});
    }
  }
  
  for(auto tt : ROOT::TSeqI(0, 8)) {
    auto resgraph = getPtResolution(hsparse, tt, cuts);
    if(resgraph) ptres[tt] = resgraph;
  }

  std::stringstream canvasname, canvastitle;
  canvasname << "ptresplotPWGJEQA";
  canvastitle << "Pt resolution comparison";
  if(trigger.length()) {
    canvasname << "_" << trigger;
    canvastitle << " " << trigger;
  }
  auto plot = new ROOT6tools::TSavableCanvas(canvasname.str().data(), canvastitle.str().data(), 800, 600);
  plot->cd();

  GraphicsPad respad(gPad);
  respad.Frame("ptrepframe", "p_{t} (GeV/c)", "#sigma(p_{t})/p_{t}", 0., 100., 0., 0.3);
  respad.Label(0.35, 0.1, 0.89, 0.15, "pp, #sqrt{s} = 13 TeV, hybrid tracks 2011");
  respad.Legend(0.15, 0.5, 0.5, 0.89);
  if(trigger.length()) respad.Label(0.65, 0.83, 0.89, 0.89, Form("Trigger: %s", trigger.data()));

  std::map<int, std::string> tttitles = { {4, "Global hybrid tracks"}, {5, "Complementary hybrid tracks"}};
  std::map<int, Style> ttstyles = {{0, {kRed, 24}}, {1, {kBlue, 25}}, {2, {kGreen, 26}}, {3, {kViolet,  26}}, {4, {kOrange, 27}}, {5, {kGray, 28}},
                                   {6, {kMagenta, 29}}, {9, {kCyan, 30}}};
  for(auto [tt, res] : ptres) {
    if(tt == 4 || tt == 5)
      respad.Draw<TGraphErrors>(res, ttstyles[tt], tttitles[tt].data());
  }
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}
