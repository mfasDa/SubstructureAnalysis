#ifndef __CLING__
#include <algorithm>
#include <memory>
#include <vector>
#include <RStringView.h>
#include <TFile.h>
#include <TGraph.h>
#include <TTree.h>
#include <TTreeReader.h>
#endif

void extractWeightsFromTree(const std::string_view filename){
  std::vector<double> weights;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  TTreeReader tr(static_cast<TTree*>(reader->Get("jetSubstructureMerged")));
  TTreeReaderValue<double> weight(tr, "PythiaWeight");
  for(auto en : tr){
    if(std::find(weights.begin(), weights.end(), *weight) == weights.end()) weights.push_back(*weight);
  }
  std::sort(weights.begin(), weights.end(), std::greater<double>());

  auto graph = new TGraph(weights.size());
  for(auto ien : ROOT::TSeqI(0, weights.size())) graph->SetPoint(ien, ien+1, weights[ien]);
  graph->SetMarkerStyle(24);
  graph->Draw("ap");
  std::unique_ptr<TFile> weightfile(TFile::Open("weights.root", "RECREATE"));
  weightfile->cd();
  graph->Write("weights");
}