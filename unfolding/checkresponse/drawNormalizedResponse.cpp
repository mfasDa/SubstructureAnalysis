#include "../../struct/ResponseReader.cxx"
#include "../../struct/GraphicsPad.cxx"
#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/unfolding.C"

void drawNormalizedResponse(const std::string_view filename){
    ResponseReader reader(filename);
    auto jetradii = reader.GetJetRadii();
    auto plot = new ROOT6tools::TSavableCanvas("NormalizedResponse", "Normalized response", 1200, 1000);
    plot->DivideSquare(jetradii.size());
    int ipad = 1;
    for(auto radius = 0.2 : jetradii) {
        auto normalized = makeNormalizedResponse(reader.GetResponseFine(radius));
        normalized->SetDirectory(nullptr);
        normalized->GetZaxis()->SetRangeUser(0, 0.2);
        normalized->SetStats(false);

        plot->cd(ipad++);
        GraphicsPad responsepad(gPad);
        responsepad.Frame(Form(""), "p_{t,det} (GeV/c)", "p_{t,part} (GeV/c)", 0., 300, 0., 600.);
        responsepad.Label(0.7, 0.15, 0.89, 0.22, Form("R=%.1f", radius));
        normalized->Draw("colzsame");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}