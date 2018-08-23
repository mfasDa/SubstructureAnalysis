#ifndef __SUBSTRUCTURETREE_C__
#define __SUBSTRUCTURETREE_C__

#ifndef __CLING__
#include <iostream>
#include <memory>
#include <string>

#include <RStringView.h>
#include <TFile.h>
#include <TKey.h>
#include <TTree.h>
#endif

#include "filesystem.C"
#include "string.C"

struct JetDef {
  std::string fJetType;
  double fJetRadius;
  std::string fTrigger;
};

JetDef getJetType(const std::string_view filetag) {
  auto tokens = tokenize(std::string(filetag), '_');
  return {tokens[0], double(std::stoi(tokens[1].substr(1)))/10., tokens[2]};
}

TTree *GetDataTree(TFile &reader) {
  TTree *result(nullptr);
  for(auto k : TRangeDynCast<TKey>(reader.GetListOfKeys())){
    if(!k) continue;
    if((contains(k->GetName(), "JetSubstructure") || contains(k->GetName(), "jetSubstructure")) 
       && (k->ReadObj()->IsA() == TTree::Class())) {
      result = dynamic_cast<TTree *>(k->ReadObj());
    }
  }
  std::cout << "Found tree with name " << result->GetName() << std::endl;
  return result;
}

std::string GetNameJetSubstructureTree(const std::string_view filename){
  std::string result;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())){
    if(!k) continue;
    if((contains(k->GetName(), "JetSubstructure") || contains(k->GetName(), "jetSubstructure")) 
       && (k->ReadObj()->IsA() == TTree::Class())) {
      result = k->GetName(); 
      break;
    }
  }
  std::cout << "Found tree with name " << result << std::endl;
  return result;
}

std::string getFileTag(const std::string_view inputfile) {
  std::string tag = basename(inputfile);
  std::cout << "Tag: " << tag << std::endl;
  tag.erase(tag.find(".root"), 5);
  tag.erase(tag.find("JetSubstructureTree_"), strlen("JetSubstructureTree_"));
  return tag;
}
 
#endif