
#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/unfolding.C"

void drawNormalizedResponseRawSimple(const std::string_view filename = "AnalysisResults.root", const std::string_view jettype = "FullJet"){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto plot = new ROOT6tools::TSavableCanvas("NormalizedResponse", "Normalized response", 1200, 1000);
    plot->Divide(2,2);
    int ipad = 1;
    for(auto radius = 0.2; radius < 0.6; radius += 0.1) {
        reader->cd(Form("EnergyScaleResults_%s_R%02d_INT7_tc200", jettype.data(), int(radius*10.)));
        auto histlist = static_cast<TList *>(gDirectory->Get(Form("EnergyScaleHists_%s_R%02d_INT7_tc200", jettype.data(), int(radius*10.))));
        auto normalized = makeNormalizedResponse(static_cast<TH2 *>(histlist->FindObject("hJetResponseFine")));
        normalized->SetDirectory(nullptr);
        normalized->SetXTitle("p_{t,det} (GeV/c)");
        normalized->SetYTitle("p_{t,part} (GeV/c)");
        normalized->GetZaxis()->SetRangeUser(0, 0.2);
        normalized->SetStats(false);

        plot->cd(ipad++);
        normalized->Draw("colz");
        (new ROOT6tools::TNDCLabel(0.7, 0.15, 0.89, 0.22, Form("R=%.1f", radius)))->Draw();
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}