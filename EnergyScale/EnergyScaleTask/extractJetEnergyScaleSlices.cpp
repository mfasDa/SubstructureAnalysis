
#ifndef __CLING__
#include <array>
#include <iomanip>
#include <memory>
#include <string>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TH2.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#endif

#include "../../helpers/graphics.C"

void extractJetEnergyScaleSlices(const std::string_view filename = "AnalysisResults.root"){
  constexpr double kVerySmall = 1e-5;
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ")),
       writer = std::unique_ptr<TFile>(TFile::Open("EnergyScaleSlices.root", "RECREATE"));
  for(const auto r : ROOT::TSeqI(0, 4)){
    double radius = 0.2 + double(r) * 0.1;
    reader->cd(Form("EnergyScaleResults_FullJet_R%02d_INT7", int(radius * 10.)));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hdiff = static_cast<THnSparse *>(histlist->FindObject("hPtDiff"));
    writer->mkdir(Form("FullJet_R%02d", int(radius * 10.)));
    writer->cd(Form("FullJet_R%02d", int(radius * 10.)));
    for(auto s : ROOT::TSeqI(10, 200, 10)) {
      double start = s, end = start + 10.;
      hdiff->GetAxis(0)->SetRangeUser(start + kVerySmall, end - kVerySmall);
      auto hslice = hdiff->Projection(2);
      hslice->SetNameTitle(Form("slice_%d_%d", int(start), int(end)), Form("Energy scale slice %.1f GeV/c to %.1f GeV/c", start, end));
      hslice->Write();
    }
  }
}