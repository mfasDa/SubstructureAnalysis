#ifndef __CLING__
#include <memory>
#include <TFile.h>
#include <TKey.h>
#endif
void combineMC(const char *mbfile, const char *trgfile){
  auto mbreader = std::unique_ptr<TFile>(TFile::Open(mbfile, "READ")),
       trgreader = std::unique_ptr<TFile>(TFile::Open(trgfile, "READ")),
       writer = std::unique_ptr<TFile>(TFile::Open("AnalysisResults_combined.root", "RECREATE"));
  
  mbreader->cd("JetConstituentQA_INT7");
  auto keys = gDirectory->GetListOfKeys();
  writer->mkdir("JetConstituentQA_INT7");
  writer->cd("JetConstituentQA_INT7");
  for(auto k : TRangeDynCast<TKey>(keys)) k->ReadObj()->Write(k->GetName(), TObject::kSingleKey);

  std::vector<std::string> triggers = {"EJ1", "EJ2"};
  for(auto t  : triggers) {
    auto keyname = "JetConstituentQA_" + t;
    trgreader->cd(keyname.data());
    keys = gDirectory->GetListOfKeys();
    writer->mkdir(keyname.data());
    writer->cd(keyname.data());
    for(auto k : TRangeDynCast<TKey>(keys)) k->ReadObj()->Write(k->GetName(), TObject::kSingleKey);
  }
}