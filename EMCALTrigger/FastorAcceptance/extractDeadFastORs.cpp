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
#endif

std::vector<int> FindDeadFastORs(const TH2 &level1spectra) {
  std::vector<int> fastOrAbsIDs;
  const int PHOSMIN = -1., PHOSMAX = -1;
  for(auto channel : ROOT::TSeqI(0, level1spectra.GetXaxis()->GetNbins())){
    if(channel >= PHOSMIN && channel <= PHOSMAX) continue;        // Do not include FastOR IDs in PHOS region
    auto specFastor = std::unique_ptr<TH1>(level1spectra.ProjectionY("py", channel+1, channel+1));
    if(!specFastor->GetEntries()) fastOrAbsIDs.emplace_back(channel);
  }
  std::sort(fastOrAbsIDs.begin(), fastOrAbsIDs.end(), std::less<int>());
  return fastOrAbsIDs;
}

void extractDeadFastORs(const std::string_view filename = "AnalysisResults.root"){
  auto filereader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  filereader->cd("fastorMonitorINT7");
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto l1timesums = static_cast<TH2 *>(histlist->FindObject("hFastOrTimeSum"));

  std::ofstream fastorwriter("maskedFastors.txt");
  for(auto id : FindDeadFastORs(*l1timesums)) fastorwriter << id << std::endl;
  fastorwriter.close();
}