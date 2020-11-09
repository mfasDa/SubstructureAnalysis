#ifndef __CLING__
#include <iostream>
#include <memory>
#include <string>
#include <RStringView.h>
#include <TFile.h>
#include <TKey.h>
#include <TTree.h>
#include <TString.h>
#endif

void store_tree(TTree *t){
  std::string fname = t->GetName();
  fname += ".root";
  std::cout  << "Storing tree under file " << fname << std::endl;

  std::unique_ptr<TFile> writer(TFile::Open(fname.data(), "RECREATE"));
  writer->cd();
  auto ctree = t->CloneTree();
  ctree->SetName("jetSubstructure");
  ctree->Write();
}

void extractSubstructureTrees(std::string_view infilename = "AnalysisResults.root"){
  std::unique_ptr<TFile> reader(TFile::Open(infilename.data(), "READ"));
  for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())){
    if(!k) continue;
    std::cout << "Found " << k->GetName() << std::endl;
    TObject *o = static_cast<TKey *>(k)->ReadObj();
    if(o->IsA() == TTree::Class()) std::cout << "Object is a tree with name " << o->GetName() << std::endl; 
    if((o->IsA() == TTree::Class()) && (TString(o->GetName()).Contains("JetSubstructure"))){
      std::cout << "Found substructure tree" << std::endl;
      store_tree(static_cast<TTree *>(o));
    }
  }
}