#ifndef __CLING__
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TList.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <TString.h>
#include <TSystem.h>
#endif

template<typename t>
std::unique_ptr<t> make_unique(t *ptr) { 
  return std::unique_ptr<t>(ptr); 
}

void ScaleList(TList &inputlist) {
  const std::array<std::string, 2> scalehists = {"fHistXsection", "fHistTrials"};
  double weight = static_cast<TH1 *>(inputlist.FindObject(scalehists[0].data()))->GetBinContent(1)/static_cast<TH1 *>(inputlist.FindObject(scalehists[1].data()))->GetBinContent(1);
  std::cout << "Applying weight " << weight << std::endl;
  for(auto o : inputlist){
    if(std::find(scalehists.begin(), scalehists.end(), o->GetName()) != scalehists.end()) continue;
    if(auto hist = dynamic_cast<TH1 *>(o)){
      hist->Scale(weight);
    } else if(auto sparse = dynamic_cast<THnSparse *>(o)){
      sparse->Scale(weight);
    }
  }
}

std::vector<std::string> GetFilesToMerge(std::string_view inputdir, std::string_view filename) {
  std::vector<std::string> filestomerge;
  auto content = make_unique<TObjArray>(TString(gSystem->GetFromPipe(Form("find %s -name %s", inputdir.data(), filename.data()))).Tokenize("\n"));
  for(auto f : *content){
    TString &fname = static_cast<TObjString *>(f)->String();
    if(fname.Contains("child")) filestomerge.emplace_back(fname);
  }
  std::cout << "Found " << filestomerge.size() << " files to merge.\n";
  return filestomerge;
}

void mergeOld(){
  std::vector<std::unique_ptr<TFile>> inputfiles;
  std::map<std::string, TList *> merged;
  std::map<std::string, std::unique_ptr<TList>> tomerge;
  for(auto f : GetFilesToMerge(gSystem->pwd(), "AnalysisResults.root")){
    std::cout << "Reading " << f << std::endl;
    auto file = make_unique<TFile>(TFile::Open(f.data(), "READ"));
    for(auto k : TRangeDynCast<TKey>(file->GetListOfKeys())){
      if(!k) continue;
      if(!TString(k->GetName()).Contains("ChargedParticleQA")) continue;  
      file->cd(k->GetName());
      auto histlist = static_cast<TList*>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
      ScaleList(*histlist);
      auto mergedlist = merged.find(k->GetName());
      if(mergedlist == merged.end()) {
        merged.insert(std::pair<std::string, TList *>(k->GetName(), histlist));
        tomerge.insert(std::pair<std::string, std::unique_ptr<TList>>(k->GetName(), std::unique_ptr<TList>(new TList)));
      } else {
        auto mergeiter = tomerge.find(k->GetName());
        mergeiter->second->Add(histlist);
      }
    }
  }
  for(auto m : merged) {
    m.second->Merge((tomerge.find(m.first)->second).get());
  }

  auto outputfile = make_unique<TFile>(TFile::Open("AnalysisResults.root", "RECREATE"));
  for(auto m : merged) {
    outputfile->mkdir(m.first.data());
    outputfile->cd(m.first.data());
    m.second->Write(m.second->GetName(), TObject::kSingleKey);
  }
}