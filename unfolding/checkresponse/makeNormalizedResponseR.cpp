#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/unfolding.C"

ROOT6tools::TSavableCanvas *makePlotJetR(TFile &reader, const std::string_view jettype, double r) {
    auto plot = new ROOT6tools::TSavableCanvas(Form("normresponse_R%02d_matchingR", int(r*10.)), Form("Normalized response matching dR R=%.1f",r), 1200, 800);
    plot->Divide(5,2);

    reader.cd(Form("EnergyScaleResults_%s_R%02d_INT7", jettype.data(), int(r*10.)));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto rawsparse = static_cast<THnSparse *>(histlist->FindObject("hPtCorr"));
    rawsparse->Sumw2();

    int ipad = 1;
    for(double rmin = 0.; rmin < 1.; rmin += 0.1) {
        plot->cd(ipad++);
        int binID = rawsparse->GetAxis(3)->FindBin(rmin+1e-5);
        rawsparse->GetAxis(3)->SetRange(binID, binID);
        std::unique_ptr<TH2> rawresponse(rawsparse->Projection(0,1));
        auto normalized = makeNormalizedResponse(rawresponse.get());
        normalized->SetDirectory(nullptr);
        normalized->SetTitle(Form("%.1f < dR < %.1f", rmin, rmin+0.1));
        normalized->SetXTitle("p_{t,det} (GeV/c)");
        normalized->SetYTitle("p_{t,part} (GeV/c)");
        normalized->GetZaxis()->SetRangeUser(0., 0.2);
        normalized->SetStats(false);
        normalized->Draw("colz");
        if(ipad == 2) (new ROOT6tools::TNDCLabel(0.7, 0.15, 0.89, 0.22, Form("R=%.1f", r)))->Draw();
    }
    plot->cd();
    plot->Update();

    return plot;
}

void makeNormalizedResponseR(const std::string_view inputfile = "AnalysisResults.root",  const std::string_view jettype = "FullJet") {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    for(double r  = 0.2; r  < 0.7; r += 0.1) {
        auto plot = makePlotJetR(*reader, jettype, r);
        plot->SaveCanvas(plot->GetName());
    }
}