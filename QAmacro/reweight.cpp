#ifndef __CLING__
#include <memory>
#include <string>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TString.h>
#endif

void reweight(std::string_view filename){
  std::string outfilename(filename);
  outfilename = outfilename.substr(0, outfilename.find_last_of(".")) + "_scaled.root";
  std::unique_ptr<TFile> fread(TFile::Open(filename.data(), "READ")),
                         fwrite(TFile::Open(outfilename.data(), "RECREATE"));
  for(auto d : *(fread->GetListOfKeys())){
    if(TString(d->GetName()).Contains("PWG")){
      fread->cd(d->GetName());
      auto histlist = static_cast<TList *>(static_cast<TKey*>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
      auto weight = static_cast<TH1 *>(histlist->FindObject("fh1Xsec"))->GetBinContent(1) / static_cast<TH1 *>(histlist->FindObject("fh1Trials"))->GetBinContent(1);

      fwrite->mkdir(d->GetName());
      fwrite->cd(d->GetName());
      for(auto h : *histlist) {
        TString histname(h->GetName());
        if(histname.Contains("fh1Xsec") || histname.Contains("fh1Trials")) continue;
        static_cast<TH1 *>(h)->Scale(weight);
      }
      histlist->Write(histlist->GetName(), TObject::kSingleKey);
    }
  }
}