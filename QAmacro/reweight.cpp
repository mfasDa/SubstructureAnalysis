#ifndef __CLING__
#include <iostream>
#include <memory>
#include <string>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TProfile.h>
#include <TString.h>
#endif

void reweight(std::string_view filename = "AnalysisResults.root"){
  std::string outfilename(filename);
  outfilename = outfilename.substr(0, outfilename.find_last_of(".")) + "_scaled.root";
  std::unique_ptr<TFile> fread(TFile::Open(filename.data(), "READ")),
                         fwrite(TFile::Open(outfilename.data(), "RECREATE"));
  for(auto d : *(fread->GetListOfKeys())){
    if(TString(d->GetName()).Contains("PWG")){
      fread->cd(d->GetName());
      auto histlist = static_cast<TList *>(static_cast<TKey*>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
      double xsec = static_cast<TH1 *>(histlist->FindObject("fh1Xsec"))->GetBinContent(1), ntrials = static_cast<TH1 *>(histlist->FindObject("fh1Trials"))->GetBinContent(1);
      auto weight = xsec / ntrials;
      std::cout << "Using weight " << weight << " (xsec " << xsec << ", ntrials " << ntrials << ")" << std::endl;

      fwrite->mkdir(d->GetName());
      fwrite->cd(d->GetName());
      for(auto h : TRangeDynCast<TH1>(histlist)) {
        if(!h) continue;
        TString histname(h->GetName());
        if(histname.Contains("fh1Xsec") || histname.Contains("fh1Trials")) continue;
        if(h->IsA() == TProfile::Class()) continue;     // don't scale profile histograms
        h->Scale(weight);
      }
      histlist->Write(histlist->GetName(), TObject::kSingleKey);
    }
  }
}