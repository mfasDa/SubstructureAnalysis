#ifndef __CLING__
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <sstream>

#include <RStringView.h>
#include "ROOT/TSeq.hxx"
#include <TFile.h>
#include <TGraph.h>
#include <TH2.h>
#include <TObjArray.h>
#include <TSystem.h>

#include "AliOADBContainer.h"
#include "AliDataFile.h"

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"

struct trendvalue {
  int runmin;
  int runmax;
  double fracEMCAL;
  double fracDCAL;
};

int getNmasked(const TH2 *modhist) {
  int nmask(0);
  for(auto ix : ROOT::TSeqI(0, modhist->GetXaxis()->GetNbins())){
    for(auto iy : ROOT::TSeqI(0, modhist->GetYaxis()->GetNbins())){
      if(modhist->GetBinContent(ix+1, iy+1)) nmask++;
    }
  }
  return nmask;
}

std::vector<trendvalue> extractMasksPeriod(int firstrun, int lastrun) {
  const int kNEMCAL = 12288, kNDCAL = 5376;
  std::vector<trendvalue> ranges;
  auto reader = std::unique_ptr<TFile>(TFile::Open(AliDataFile::GetFileNameOADB("EMCAL/EMCALBadChannels.root").data(), "READ"));
  auto cont = static_cast<AliOADBContainer *>(reader->Get("AliEMCALBadChannels"));
  for(auto r : ROOT::TSeqI(0, cont->GetNumberOfEntries())){
    if((cont->UpperLimit(r) >= firstrun && cont->UpperLimit(r) <= lastrun) || (cont->LowerLimit(r) >= firstrun && cont->LowerLimit(r) <= lastrun)){
      auto rangedata = static_cast<TObjArray *>(cont->GetObjectByIndex(r));
      int nemcal(0), ndcal(0);
      for(auto sm : ROOT::TSeqI(0, 12)) nemcal += getNmasked(static_cast<TH2 *>(rangedata->FindObject(Form("EMCALBadChannelMap_Mod%d", sm))));
      for(auto sm : ROOT::TSeqI(12, 20)) ndcal += getNmasked(static_cast<TH2 *>(rangedata->FindObject(Form("EMCALBadChannelMap_Mod%d", sm))));
      ranges.push_back({cont->LowerLimit(r), cont->UpperLimit(r), double(nemcal)/double(kNEMCAL), double(ndcal)/double(kNDCAL)});
    }
  }
  return ranges;
}

std::set<int> getListOfRuns(const std::string_view period){
  std::set<int> result;
  std::string repo = "/data1/markus/Fulljets/pp_13TeV/Substructuretree/code/runlists_EMCAL";
  std::stringstream pathname;
  pathname << repo << "/" << period;
  if(gSystem->AccessPathName(pathname.str().data())) return result;
  std::ifstream reader(pathname.str());
  std::string line;
  while(std::getline(reader, line)){
    std::stringstream decoder(line);
    std::string token;
    while(std::getline(decoder, token, ',')) result.insert(std::stoi(token));
  }
  reader.close();
  return result;
}

void makeTrendFEEMask(const std::string_view period){
  auto runs = getListOfRuns(period);
  if(!runs.size()){
    std::cout << "No runs found for period " << period << std::endl;
    return;
  }
  
  auto trend = extractMasksPeriod(*runs.begin(), *runs.rbegin());
  std::cout << "Found " << trend.size() << " range(s)" << std::endl;
  auto emcaltrend = new TGraph, dcaltrend = new TGraph;
  for(auto r : runs) {
    auto v = std::find_if(trend.begin(), trend.end(), [r](const trendvalue &tv) -> bool { return r >= tv.runmin && r <= tv.runmax; });
    if(v == trend.end()) continue;
    emcaltrend->SetPoint(emcaltrend->GetN(), r, v->fracEMCAL);
    dcaltrend->SetPoint(dcaltrend->GetN(), r, v->fracDCAL);
  }

  auto plot = new ROOT6tools::TSavableCanvas(Form("FEEtrend%s", period.data()), Form("FEE trending for period %s", period.data()), 800, 600);
  plot->cd();

  (new ROOT6tools::TAxisFrame(Form("frame%s", period.data()), "run", "fraction masked", double(*runs.begin()), double(*runs.rbegin()), 0., .5))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.7, 0.75, 0.89, 0.89);
  leg->Draw();
  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.3, 0.89, Form("Period: %s", period.data())))->Draw();

  Style{kRed, 24}.SetStyle<TGraph>(*emcaltrend);
  emcaltrend->Draw("lpsame");
  leg->AddEntry(emcaltrend, "EMCAL", "lp");
  Style{kBlue, 25}.SetStyle<TGraph>(*dcaltrend);
  dcaltrend->Draw("lpsame");
  leg->AddEntry(dcaltrend, "DCAL", "lp");

  plot->Update();
  plot->SaveCanvas(plot->GetName());

  std::unique_ptr<TFile> datawriter(TFile::Open(Form("feetrend_%s.root", period.data()), "RECREATE"));
  datawriter->cd();
  emcaltrend->Write("emcaltrend");
  dcaltrend->Write("dcaltrend");
}