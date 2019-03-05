#include "../../meta/stl.C"
#include "../../meta/root.C"

void makeTrackingEffPWGJEQA(const std::string_view inputfile = "AnalysisResults.root") {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    auto histlist = static_cast<TList *>(reader->Get("AliEmcalTrackingQATask_histos"));
    auto hpart = static_cast<THnSparse *>(histlist->FindObject("fParticlesPhysPrim")), 
         hdet = static_cast<THnSparse *>(histlist->FindObject("fParticlesMatched"));
    //hpart->Sumw2();
    //hdet->Sumw2();
    hpart->GetAxis(4)->SetRange(2,2);
    auto specPtPart = hpart->Projection(0),
         efficiency = hdet->Projection(0);
    //specPtPart->Sumw2();
    //efficiency->Sumw2();
    efficiency->SetNameTitle("efficiency", "Tracking efficiency");
    efficiency->Divide(efficiency, specPtPart, 1., 1., "b");

    std::unique_ptr<TFile> writer(TFile::Open("trackingEfficiency_full.root", "RECREATE"));
    efficiency->Write();
}