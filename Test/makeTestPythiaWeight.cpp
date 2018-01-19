#ifndef __CLING__
#include <algorithm>
#include <functional>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TAxis.h>
#include <TFile.h>
#include <TGraph.h>
#include <TTree.h>
#include <TTreeReader.h>
#endif

void makeTestPythiaWeight(std::string_view inputfile) {
    auto filereader = TFile::Open(inputfile.data());
    TTreeReader reader(static_cast<TTree *>(filereader->Get("jetSubstructureMerged")));
    TTreeReaderValue<double> pythiaweight(reader, "PythiaWeight");

    std::vector<double> weights;
    for(auto en : reader) {
      auto eventweight = *pythiaweight;
      if(std::find(weights.begin(), weights.end(), eventweight) == weights.end()) weights.emplace_back(eventweight);
    }
    std::sort(weights.begin(), weights.end(), std::greater<double>());
    std::cout << "Found different weights for " << weights.size() << " pt-hard bins\n";

    auto weightplot = new TGraph();
    weightplot->SetMarkerColor(kBlack);
    weightplot->SetMarkerStyle(20);
    weightplot->GetXaxis()->SetTitle("p_{t,hard}-bin");
    weightplot->GetYaxis()->SetTitle("weight");

    for(auto bin : ROOT::TSeqI(0, weights.size())){
      weightplot->SetPoint(bin, bin, weights[bin]);
      std::cout << "weight bin " << bin << ": " << std::scientific << weights[bin] << std::endl;
    }

    weightplot->Draw("ap");
}