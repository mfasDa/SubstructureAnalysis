#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/unfolding.C"

void drawNormalizedResponse(const std::string_view filename){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto plot = new ROOT6tools::TSavableCanvas("NormalizedResponse", "Normalized response", 1200, 1000);
    plot->Divide(2,2);
    int ipad = 1;
    for(auto radius = 0.2; radius < 0.6; radius += 0.1) {
        reader->cd(Form("R%02d", int(radius*10.)));
        gDirectory->cd("response");
        TH2 *rawresponse = static_cast<TH2 *>(gDirectory->Get(Form("Rawresponse_R%02d", int(radius*10))));
        auto normalized = makeNormalizedResponse(rawresponse);
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