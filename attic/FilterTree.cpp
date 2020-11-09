#ifndef __CLING__
#include <memory>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TKey.h>
#include <TString.h>
#include <TTree.h>
#include <TTreeReader.h>
#endif

void FilterTree(std::string_view inputfile, double ptcut = 10.){
  auto newfilename = inputfile.substr(0, inputfile.find_last_of(".")) + "_filtered.root";
  std::cout << "Writing output to " << newfilename << std::endl;

  std::unique_ptr<TFile> infilereader(TFile::Open(inputfile.data(), "READ"));
  TTree *substructuretree(nullptr);
  for(auto t : *(infilereader->GetListOfKeys())){
    if(TString(t->GetName()).Contains("jetSubstructure")){
      substructuretree = static_cast<TTree *>(static_cast<TKey *>(t)->ReadObj());
    }
  }
  if(!substructuretree){
    std::cerr << "No substructure tree found in file " << inputfile << std::endl;
    return;
  }
  std::cout << "Found tree " << substructuretree->GetName() << " in file " << inputfile << std::endl;
  TTreeReader tread(substructuretree);
  std::vector<TString> varnamesNew;
  auto cutbranch = 0;
  for(auto b : *(tread.GetTree()->GetListOfBranches())){
    varnamesNew.emplace_back(static_cast<TBranch *>(b)->GetName());
    std::cout << "Last branch: " << varnamesNew.back() << std::endl;
    if(varnamesNew.back() == "PtJetRec") {
      cutbranch = varnamesNew.size() - 1;
      std::cout << "Found cut branch at position" << cutbranch << std::endl;
    }
  }
  std::cout << "Found " << varnamesNew.size() << " branches ...\n";
  std::vector<std::unique_ptr<TTreeReaderValue<double>>> inputvalues;
  for(auto ivar : varnamesNew){
    inputvalues.emplace_back(std::unique_ptr<TTreeReaderValue<double>>(new TTreeReaderValue<double>(tread, ivar.Data())));
  }

  std::unique_ptr<TFile> writer(TFile::Open(newfilename, "RECREATE"));
  writer->cd();
  TTree *outputtree = new TTree("jetSubstructureFiltered", "filtered jet substructure tree");
  double *jettreevalues = new double[varnamesNew.size()];
  for(auto ib : ROOT::TSeqI(0, varnamesNew.size())){
    outputtree->Branch(varnamesNew[ib], jettreevalues + ib, Form("%s/D", varnamesNew[ib].Data()));
  }

  bool debug = false;
  for(auto en : tread) {
    TTreeReaderValue<double> &cutvalue = *(inputvalues[cutbranch]);
    auto simvalue =*(*(inputvalues[3]));
    auto dolog = debug && simvalue > 10.;
    if(dolog) std::cout << "Found cut branch " << cutvalue.GetBranchName() << " with value " << *cutvalue << " (sim " <<  simvalue << ")";  
    if(*cutvalue < ptcut) {
      if(dolog) std::cout << " ... rejected" << std::endl;
      continue; 
    } else {
      if(dolog) std::cout << " ... accepted" << std::endl;
    }

    if(dolog) std::cout << "Filling new tree" << std::endl;
    for(auto ib : ROOT::TSeqI(0, varnamesNew.size())) {
      jettreevalues[ib] = *(*(inputvalues[ib]));
    }
    outputtree->Fill();
  }
  outputtree->Write();
  delete[] jettreevalues;
}