#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/math.C"

void extractNormalizedSpectrum(const std::string_view filename = "AnalysisResults.root"){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto histos = static_cast<TH1 *>(reader->Get("AliEmcalTrackingQATask_histos"));
    auto hTracks = static_cast<THnSparse *>(histos->FindObject("fTracks"));
    auto hNorm = static_cast<TH1 *>(histos->FindObject("fHistEventCount"));
    auto norm = hNorm->GetBinContent(hNorm->GetXaxis()->FindBin("Accepted"));
    auto spectrum = hTracks->Projection(0);
    spectrum->SetName("spectrum");
    spectrum->SetDirectory(nullptr);
    normalizeBinWidth(spectrum);
    spectrum->Scale(1./norm);
    std::unique_ptr<TFile> writer(TFile::Open("trackYield_hybrid_MB_full.root", "RECREATE"));
    spectrum->Write();
}