#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/unfolding.C"

TH2 *makeNormalizedResponseSwap(TH2 *responsein) {
    auto normalised = static_cast<TH2 *>(histcopy(responsein));
    std::unique_ptr<TH1> projected(responsein->ProjectionX("_px", 1, responsein->GetNbinsX()));
    for(auto ptpart : ROOT::TSeqI(0, responsein->GetNbinsX())) {
        auto weight = projected->GetBinContent(ptpart+1);
        if(weight){
            for(auto ptdet : ROOT::TSeqI(0, responsein->GetNbinsY())){
                auto bc = responsein->GetBinContent(ptpart+1, ptdet+1);
                normalised->SetBinContent(ptpart+1, ptdet+1, bc/weight);
            }
        }
    }
    return normalised;
}

void drawNormalizedResponseOld(){
    auto plot = new ROOT6tools::TSavableCanvas("NormalizedResponse", "Normalized response", 1200, 1000);
    plot->Divide(2,2);
    int ipad = 1;
    for(auto radius = 0.2; radius < 0.6; radius += 0.1) {
        std::unique_ptr<TFile> reader(TFile::Open(Form("responsematrixpt_FullJets_R%02d_INT7_merged.root", int(radius*10.)), "READ"));
        TH2 *rawresponse = static_cast<TH2*>(reader->Get("matrixfine_allptsim"));
        auto normalized = makeNormalizedResponseSwap(rawresponse);
        normalized->SetDirectory(nullptr);
        normalized->SetYTitle("p_{t,det} (GeV/c)");
        normalized->SetXTitle("p_{t,part} (GeV/c)");
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