#ifndef __CLING__
#include <memory>
#include <string>
#include <sstream>
#include <TFile.h>
#include <TKey.h>
#include <TList.h>
#endif

#include "../../helpers/filesystem.C"

void extractTriggerCorrelation(const char *mydirname, const char *filename = "AnalysisResults.root"){
  std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
  reader->cd(mydirname);
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto trghist = histlist->FindObject("hTriggerCorrelation");

  std::stringstream outfilename;
  auto directory = dirname(filename);
  if(directory.length()) outfilename << directory << "/";
  outfilename << "TriggerCorrelation.root";

  std::unique_ptr<TFile> writer(TFile::Open(outfilename.str().data(), "RECREATE"));
  writer->cd();
  trghist->Write();
}