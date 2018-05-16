#ifndef __CLING__
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <RStringView.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH2.h>
#include <TSystem.h>
#endif

#include "../../helpers/string.C"

std::tuple<double, double> extractFractionsInMinBias(const std::string_view filename){
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  auto hcorr = static_cast<TH2 *>(reader->Get("hTriggerCorrelation"));
  auto mbbin = hcorr->GetYaxis()->FindBin("MB");
  auto mbslice = std::unique_ptr<TH1>(hcorr->ProjectionX("mbslice", mbbin, mbbin));
  mbslice->Scale(1./mbslice->GetBinContent(mbbin));
  return std::make_tuple<double, double>(mbslice->GetBinContent(mbslice->GetXaxis()->FindBin("EG1")), mbslice->GetBinContent(mbslice->GetXaxis()->FindBin("EJ1")));
}

std::vector<int> getListOfRuns(const std::string_view inputdir){
  std::string dirs = gSystem->GetFromPipe(Form("ls -1 %s", inputdir.length() ? inputdir.data() : gSystem->pwd())).Data();
  std::vector<int> runs;
  for(auto d : tokenize(dirs)){
    if(is_number(d)) {
      if(!gSystem->AccessPathName(Form("%s/TriggerCorrelation.root", d.data()))) runs.emplace_back(std::stoi(d));
    }
  }
  std::sort(runs.begin(), runs.end(), std::less<int>());
  return runs;
}

void trendFractionInMinBias(const std::string_view inputdir = ""){
  TGraph *trendFractionEG1 = new TGraph, *trendFractionEJ1 = new TGraph;
  for(auto r : getListOfRuns(inputdir)){
    std::cout << "Reading run " << r << std::endl;
    std::stringstream rfile;
    if(inputdir.length()) rfile << inputdir << "/";
    rfile << std::setw(9) << std::setfill('0') << r << "/TriggerCorrelation.root";
    auto data =  extractFractionsInMinBias(rfile.str());
    trendFractionEG1->SetPoint(trendFractionEG1->GetN(), static_cast<double>(r), std::get<0>(data));
    trendFractionEJ1->SetPoint(trendFractionEJ1->GetN(), static_cast<double>(r), std::get<1>(data));
  }

  std::unique_ptr<TFile> trendwriter(TFile::Open("trendFracTriggerMB.root", "RECREATE"));
  trendwriter->cd();
  trendFractionEG1->Write("trendEG1");
  trendFractionEJ1->Write("trendEJ1");
}