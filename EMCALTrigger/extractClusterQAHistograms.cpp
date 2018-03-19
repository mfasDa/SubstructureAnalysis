#ifndef __CLING__
#include <array>
#include <memory>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>
#endif

std::vector<TH1 *> GetNormalizedClusterSpectra(TList *histos) {
  auto binnorm = [](TH1 *hist) {
    for(auto b : ROOT::TSeqI(1, hist->GetXaxis()->GetNbins()+1)) {
      auto bw = hist->GetXaxis()->GetBinWidth(b);
      hist->SetBinContent(b, hist->GetBinContent(b)/bw);
      hist->SetBinError(b, hist->GetBinError(b)/bw);
    }
  };
  std::vector<TH1 *> result;
  std::array<std::string, 9> triggers = {{"MB", "EG1", "EG2", "EJ1", "EJ2", "DG1", "DG2", "DJ1", "DJ2"}}; 
  for(auto trg : triggers) {
    auto norm = static_cast<TH1 *>(histos->FindObject(Form("hEventCount%s", trg.data())));
    auto raw = static_cast<TH2 *>(histos->FindObject(Form("hClusterEnergySM%s", trg.data())));
    auto EMCAL = raw->ProjectionY(Form("specEMCAL_%s", trg.data()), 1, 12),
         DCAL = raw->ProjectionY(Form("specDCAL_%s", trg.data()), 13, 20);
    EMCAL->Scale(1./norm->GetBinContent(1));
    DCAL->Scale(1./norm->GetBinContent(1));
    binnorm(EMCAL);
    binnorm(DCAL);
    result.emplace_back(EMCAL);
    result.emplace_back(DCAL);
  }
  return result;
}

void extractClusterQAHistograms(const char *inputfile = "AnalysisResults.root"){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ")),
                         writer(TFile::Open("ClusterQA.root", "RECREATE"));
  for(auto k : *reader->GetListOfKeys()){
    if(TString(k->GetName()).Contains("ClusterQA")) {
      writer->mkdir(k->GetName());

      reader->cd(k->GetName());
      auto histlist = GetNormalizedClusterSpectra(static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj()));

      writer->cd(k->GetName());
      TList *histos = new TList;
      histos->SetName("histos");
      histos->SetOwner(true);
      for(auto h : histlist) histos->Add(h);
      histos->Write(histos->GetName(), TObject::kSingleKey);
    }
  }
}