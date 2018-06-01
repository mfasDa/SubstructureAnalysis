#ifndef __CLING__
#include <array>
#include <memory>
#include <sstream>
#include <RStringView.h>
#include <TFile.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>
#endif

#include "../../helpers/math.C"
#include "../../helpers/filesystem.C"

void extractClusterSpectraOld(const std::string_view filename = "AnalysisResults.root"){
  std::stringstream outfilename;
  auto datadir = dirname(filename);
  if(datadir.length() && (datadir != std::string_view("."))) outfilename << datadir << "/";
  outfilename << "ClusterSpectra_INT7.root";
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ")),
                         writer(TFile::Open(outfilename.str().data(), "RECREATE"));
  writer->mkdir("ANY");

  reader->cd("ClusterQA_Default");
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());

  std::array<std::string, 11> triggers = {{"MB", "EMC7", "EG1", "EG2", "EJ1", "EJ2", "DMC7", "DG1", "DG2", "DJ1", "DJ2"}};
  writer->cd("ANY");
  for(auto t : triggers){
    auto norm = static_cast<TH1 *>(histlist->FindObject(Form("hEventCount%s", t.data())));
    auto spec2D = static_cast<TH2 *>(histlist->FindObject(Form("hClusterEnergySM%s", t.data())));

    if(t == "MB") {
      // spectra for both sides
      auto emcalspectrum = spec2D->ProjectionY("clusterSpectrum_EMCAL_MB_ANY", 1, 12);
      normalizeBinWidth(emcalspectrum);
      emcalspectrum->Scale(1./norm->GetBinContent(1));
      emcalspectrum->Write();
      auto dcalspectrum = spec2D->ProjectionY("clusterSpectrum_DCAL_MB_ANY", 13, 20);
      normalizeBinWidth(dcalspectrum);
      dcalspectrum->Scale(1./norm->GetBinContent(1));
      dcalspectrum->Write();
    } else {
      int binmin = t[0] == 'E' ? 1 : 12, binmax = t[0] == 'E' ? 11 : 20;
      auto trgspectrum = spec2D->ProjectionY(Form("clusterSpectrum_%s_%s_ANY", t[0] == 'E' ? "EMCAL" : "DCAL", t.data()), binmin, binmax);
      normalizeBinWidth(trgspectrum);
      trgspectrum->Scale(1./norm->GetBinContent(1));
      trgspectrum->Write();
    }
  }
}