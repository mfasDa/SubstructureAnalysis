//////Full detector simulation//////////////////////
#ifndef __CLING__
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include "TList.h"
#include "ROOT/TSeq.hxx"
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

std::vector<int> GetPtHardBins(const char *inputdir, bool &usechilds){
  std::vector<int> pthardbins;
  TString dirstring = gSystem->GetFromPipe(Form("ls -1 %s", inputdir));
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
  return pthardbins;
}

void MergeResponsepp(const char *inputdir, const char *treefile, const char *histfile){
  TString basename(treefile);
  basename.ReplaceAll(".root", "");
  TString dirname(basename);
  dirname.ReplaceAll("Tree", "");
  dirname.ReplaceAll("_filtered", "");

  bool usechilds(false);
  auto pthardbins = GetPtHardBins(inputdir, usechilds);
  std::sort(pthardbins.begin(), pthardbins.end(), std::less<int>());
  std::cout << "Found " << pthardbins.size() << " pt-hard bins" << std::endl;
  TList weighted;
  std::vector<TFile *> treefiles;

  Double_t pythiaweight(0.); // Variable needde for new branch
  std::unique_ptr<TFile> output(TFile::Open(Form("%s_merged.root", basename.Data()), "RECREATE"));

  for(auto b : pthardbins){
    // Read in weights
    TString inputfile = usechilds ? Form("%s/child_%d/%s", inputdir, b, histfile) : Form("%s/%02d/%s", inputdir, b, histfile);
    std::unique_ptr<TFile> filereader(TFile::Open(inputfile, "READ")); 
    filereader->cd(dirname);
    TList *histos = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
    int databin = usechilds ? 1 : b+1;
    auto xsection = static_cast<TH1 *>(histos->FindObject("fHistXsection"))->GetBinContent(databin);
    auto nTrials = static_cast<TH1 *>(histos->FindObject("fHistTrials"))->GetBinContent(databin);
    auto nEvents = static_cast<TH1 *>(histos->FindObject("fHistEvents"))->GetBinContent(databin);
    auto weight = xsection / nTrials;
    Printf("weight: %e, xsec: %f mb, Events / trials: %f bin: %d", weight, xsection, nEvents/nTrials, b);

    // Read tree, reweight
    TString treefilename = usechilds ? Form("%s/child_%d/%s", inputdir, b, treefile) : Form("%s/%02d/%s", inputdir, b, treefile);
    TFile *treereader = TFile::Open(treefilename, "READ");
    treefiles.push_back(treereader);
    TTree *substructuretree(nullptr);
    for(auto k : TRangeDynCast<TKey>(treereader->GetListOfKeys())){
      if(!k) continue;
      std::cout << "Processing key " << k->GetName() << std::endl;
      TString keyname(k->GetName());
      if(keyname.Contains("jetSubstructure") || keyname.Contains("JetSubstructure")){
        substructuretree = k->ReadObject<TTree>();
        break;
      }
    }
    if(substructuretree) std::cout << "Found substructuretree " << substructuretree->GetName() << " in file " << treefilename << std::endl;
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
  printf("Before merging\n");
  
  output->cd();
  TTree *outputtree = TTree::MergeTrees(&weighted);
  printf("After merging\n");
  outputtree->SetName("jetSubstructureMerged");
  outputtree->Write();

  for(auto f : treefiles) f->Close();
}
