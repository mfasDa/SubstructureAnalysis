//////Full detector simulation//////////////////////
#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <tuple>
#include "TList.h"
#include "ROOT/TSeq.hxx"
#include <RStringView.h>
#include <TKey.h>
#include "TSystem.h"
#include "TFile.h"
#include "TDirectory.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"
#include "TString.h"
#include "TProfile.h"
#include "TCanvas.h"
#endif

std::tuple<std::vector<int>, bool> GetPtHardBins(std::string_view inputdir){
  std::vector<int> pthardbins;
  bool usechilds(false);
  TString dirstring = gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data()));
  std::unique_ptr<TObjArray> dirs(dirstring.Tokenize("\n"));
  for(auto d : TRangeDynCast<TObjString>(dirs.get())){
    if(!d) continue;
    TString mydir = d->String();
    if(mydir.IsDigit()) {
      pthardbins.emplace_back(mydir.Atoi());
      usechilds = false;
    } else {
      if(mydir.Contains("child_")){
        mydir.ReplaceAll("child_", "");
        pthardbins.emplace_back(mydir.Atoi());
        usechilds = true;
      }
    }
  }
  return std::make_tuple(pthardbins, usechilds);
}

void MergeResponseppNew(std::string_view inputdir, std::string_view treename, std::string_view rootfile = "AnalysisResults.root"){
  auto respthardbins = GetPtHardBins(inputdir);
  auto pthardbins = std::get<0>(respthardbins);
  auto usechilds = std::get<1>(respthardbins);
  std::sort(pthardbins.begin(), pthardbins.end(), std::less<int>());
  std::cout << "Found " << pthardbins.size() << " pt-hard bins" << std::endl;
  TList weighted;
  std::vector<std::shared_ptr<TFile>> treefiles;

  Double_t pythiaweight(0.); // Variable needde for new branch
  std::unique_ptr<TFile> output(TFile::Open(Form("%s_merged.root", treename.data()), "RECREATE"));
  TString dirname(treename);
  dirname.ReplaceAll("Tree", "");

  for(auto b : pthardbins){
    // Read in weights
    TString inputfile = usechilds ? Form("%s/child_%d/%s", inputdir.data(), b, rootfile.data()) : Form("%s/%02d/%s", inputdir.data(), b, rootfile.data());
    std::shared_ptr<TFile> filereader(TFile::Open(inputfile, "READ")); 
    treefiles.push_back(filereader);
    filereader->cd(dirname);
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    int databin = usechilds ? 1 : b+1;
    auto xsection = static_cast<TH1 *>(histos->FindObject("fHistXsection"))->GetBinContent(databin);
    auto nTrials = static_cast<TH1 *>(histos->FindObject("fHistTrials"))->GetBinContent(databin);
    auto nEvents = static_cast<TH1 *>(histos->FindObject("fHistEvents"))->GetBinContent(databin);
    auto weight = xsection / nTrials;
    Printf("weight: %e, xsec: %f mb, Events / trials: %f bin: %d", weight, xsection, nEvents/nTrials, b);

    // Read tree, reweight
    auto substructuretree = static_cast<TTree * >(filereader->Get(treename.data()));
    if(substructuretree) std::cout << "Found substructuretree " << substructuretree->GetName() << " in file " << inputfile << std::endl;
    TBranch *bWeightPtHard = substructuretree->Branch("PythiaWeight", &pythiaweight, "PythiaWeight/D");
    for(auto en : ROOT::TSeqI(0, substructuretree->GetEntriesFast())){
      //printf("Doing entry %d\n", en);
      substructuretree->GetEntry(en);
      pythiaweight = weight;
      bWeightPtHard->Fill();
    }
    substructuretree->SetName(Form("jetSubstructure%d", b));
    output->cd();
    substructuretree->CloneTree(-1, "fast");
    weighted.Add(substructuretree);
  }
  std::cout << "Before merging\n";
  
  output->cd();
  TTree *outputtree = TTree::MergeTrees(&weighted);
  std::cout << "After merging\n";
  outputtree->SetName("jetSubstructureMerged");
  outputtree->Write();
}
