#ifndef __CLING__
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TList.h>
#endif

#include "../../helpers/math.C"

std::vector<TH1 *> makeNormalizedClusterSpectra(THnSparse * clustersparse, const TH1 *norm, bool emcal, const std::string_view trigger) {
  std::map<std::string, int> clusterindices = {{"ANY", 0}, {"CENT", 1}, {"CENTNOTRD", 2}, {"CENTBOTH", 3}, {"ONLYCENT", 4}, {"ONLYCENTNOTRD", 5}};
  std::vector<TH1 *> result;

  clustersparse->GetAxis(0)->SetRange(emcal ? 1 : 13, emcal ? 12 : 20);

  for(auto trgclst : clusterindices) {
    std::stringstream histname;
    histname << "clusterSpectrum_" << (emcal ? "EMCAL" : "DCAL") << "_" << trigger << "_" << trgclst.first;
    clustersparse->GetAxis(5)->SetRange(trgclst.second + 1, trgclst.second + 1);
    auto spec = clustersparse->Projection(1);
    clustersparse->GetAxis(5)->SetRange(0, clustersparse->GetAxis(5)->GetNbins());
    spec->SetDirectory(nullptr);
    spec->SetName(histname.str().data());
    auto normval = norm->GetBinContent(trgclst.second + 1);
    if(normval) spec->Scale(1./normval);
    normalizeBinWidth(spec);
    result.emplace_back(spec);
  }
  clustersparse->GetAxis(0)->SetRange(0, clustersparse->GetAxis(0)->GetNbins());

  return result;
}

void extractClusterSpectra(const std::string_view tag = "Default", const std::string_view filename = "AnalysisResults.root"){
  const std::array<const std::string, 6> kTriggerClusters = {{"ANY", "CENT", "CENTNOTRD", "CENTBOTH", "ONLYCENT", "ONLYCENTNOTRD"}};
  const std::array<const std::string, 9> kTriggerClasses = {{"MB", "EG1", "EG2", "EJ1", "EJ2", "DG1", "DG2", "DJ1", "DJ2"}};

  std::unique_ptr<TFile> writer(TFile::Open(Form("ClusterSpectra_%s.root", tag.data()), "RECREATE"));
  for(const auto & tcl : kTriggerClusters) writer->mkdir(tcl.data());
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));  
  reader->cd(Form("ClusterQA_%s", tag.data()));
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  std::vector<TObject *> histcont;
  for(auto o : *histlist) histcont.emplace_back(o);
  //histlist->Print();
  for(const auto &tls : kTriggerClasses) {
    if(tls != "MB") {
      auto spectra = makeNormalizedClusterSpectra(static_cast<THnSparse *>(
        *std::find_if(histcont.begin(), histcont.end(), [&tls](const TObject *o) -> bool { 
          TString histname(o->GetName()); return histname.Contains("hClusterTHnSparseAll") && histname.Contains(tls);
        })), static_cast<TH1 *>(
        *std::find_if(histcont.begin(), histcont.end(), [&tls](const TObject *o) -> bool {
          TString histname(o->GetName()); return histname.Contains("hTrgClustCounter") && histname.Contains(tls);   
        })), tls[0] == 'E', tls);
      for(auto s : spectra) std::cout << "histo " << s->GetName() << std::endl;
      for(auto tcl : kTriggerClusters){
        writer->cd(tcl.data());
        auto spec = *(std::find_if(spectra.begin(), spectra.end(), [&tcl](const TH1 *hist) -> bool { return std::string_view( hist->GetName()).find(tcl) != std::string::npos; }));
        spec->Write();
      }
    } else {
      std::array<bool, 2> detcases = {{true, false}};
      for(auto d : detcases){
        auto spectra = makeNormalizedClusterSpectra(static_cast<THnSparse *>(
        *std::find_if(histcont.begin(), histcont.end(), [&tls](const TObject *o) -> bool { 
          TString histname(o->GetName()); return histname.Contains("hClusterTHnSparseAll") && histname.Contains(tls);
        })), static_cast<TH1 *>(
        *std::find_if(histcont.begin(), histcont.end(), [&tls](const TObject *o) -> bool {
          TString histname(o->GetName()); return histname.Contains("hTrgClustCounter") && histname.Contains(tls);   
         })), d, tls);
        for(auto s : spectra) std::cout << "histo " << s->GetName() << std::endl;
        for(auto tcl : kTriggerClusters){
          writer->cd(tcl.data());
          auto spec = *(std::find_if(spectra.begin(), spectra.end(), [&tcl](const TH1 *hist) -> bool { return std::string_view( hist->GetName()).find(tcl) != std::string::npos; }));
          spec->Write();
        }
      }
    }
  }
}