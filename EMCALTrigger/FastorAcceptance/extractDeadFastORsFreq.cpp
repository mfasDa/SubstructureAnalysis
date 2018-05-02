#ifndef __CLING__
#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>
#include <TVector.h>
#include "AliEMCALGeometry.h"
#endif

#include "TStatToolkit.h"

std::vector<int> FindDeadFastORs(const TH2 &trgspectra, double threshold = 0) {
  std::vector<int> fastOrAbsIDs;
  auto geo = AliEMCALGeometry::GetInstanceFromRunNumber(260000);   // run2 geometry
  for(auto col : ROOT::TSeqI(0, 48)){
    for(auto row : ROOT::TSeqI(0, 104)){
      if(row >= 64 && row < 100 && col >= 16 && col < 32) continue;      // cut out PHOS region
      if(trgspectra.GetBinContent(col+1, row+1) >threshold) continue;
      int fabsid = -1;
      geo->GetTriggerMapping()->GetAbsFastORIndexFromPositionInEMCAL(col, row, fabsid);
      fastOrAbsIDs.emplace_back(fabsid);
    }
  }
  std::sort(fastOrAbsIDs.begin(), fastOrAbsIDs.end(), std::less<int>());
  return fastOrAbsIDs;
}

double GetThreshold(const TH2 &freqhist){
  // calculate truncated mean for threshold calculation
  std::vector<double> channelcounts;  // exclude dead channels;
  for(auto col : ROOT::TSeqI(0, 48)){
    for(auto row : ROOT::TSeqI(0, 104)){
      auto val = freqhist.GetBinContent(col, row);
      if(val == 0) continue;
      if(row >= 64 && row < 100 && col >= 16 && row < col) continue;      // cut out PHOS region
      channelcounts.emplace_back(val);
    }
  }
  std::cout << "Detected " << channelcounts.size() << " values" << std::endl;
  Double_t mean, sigma;
  TStatToolkit::EvaluateUni(channelcounts.size(), channelcounts.data(), mean, sigma, 0);  
  std::cout << "Detected uni mean at " << mean << " counts (+- " << sigma << ")" << std::endl;
  return mean * 0.1;
}

void extractDeadFastORsFreq(int triggerlevel = 1, const char *trigger = "INT7", const std::string_view filename = "AnalysisResults.root"){
  auto filereader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  filereader->cd(Form("fastorMonitor%s", trigger));
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto freqhist = static_cast<TH2 *>(histlist->FindObject(Form("hFastOrColRowFrequencyL%d", triggerlevel)));

  double threshold = (triggerlevel == 1) ? GetThreshold(*freqhist) : 0.;
  std::ofstream fastorwriter(Form("maskedFastorsFreq_L%d_%s.txt", triggerlevel, trigger));
  for(auto id : FindDeadFastORs(*freqhist, threshold)) fastorwriter << id << std::endl;
  fastorwriter.close();
}