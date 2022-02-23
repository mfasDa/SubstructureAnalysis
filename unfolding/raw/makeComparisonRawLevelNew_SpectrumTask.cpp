#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"

void DrawSpectra(const std::map<std::string, TH1 *> &spectra, Double_t r, Bool_t doleg, Bool_t rebinned) {
    gPad->SetLogy();
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    (new ROOT6tools::TAxisFrame(Form("axispec_R%02d_%s", int(r*10), (rebinned ? "rebinned" : "fine")), "p_{t} (GeV/c)", "dN/dp_{t} ((GeV/c)^{-1})", 0., 200, 1e-9, 1))->Draw("axis");
    TLegend *leg(nullptr);
    if(doleg) {
        leg = new ROOT6tools::TDefaultLegend(0.7, 0.65, 0.89, 0.89);
        leg->Draw();
    }
    (new ROOT6tools::TNDCLabel(0.2, 0.15, 0.4, 0.22, Form("R=%.1f", r)))->Draw();
    for(auto [t, s] : spectra) {
        s->Draw("epsame");
        if(leg) leg->AddEntry(s, t.data(), "lep");
    }
    gPad->Update();
}

void DrawRatios(const std::map<std::string, TH1 *> &spectra, Double_t r, Bool_t rebinned) {
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    (new ROOT6tools::TAxisFrame(Form("ratioTriggerMinBias_R%02d%s", int(r*10), (rebinned ? "rebinned" : "fine")), "p_{t} (GeV/c)", "Trigger / min. bias", 0., 200., 0., 2.))->Draw("axis");
    TLegend *leg(nullptr);
    if(rebinned) {
        leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.89, 0.3);
        leg->Draw();
    }
    std::array<std::string, 2> emcaltriggers = {{"EJ1", "EJ2"}};
    auto ref = spectra.find("INT7")->second;
    for(const auto &et : emcaltriggers){
        auto ratio = histcopy(spectra.find(et)->second);
        ratio->SetDirectory(nullptr);
        ratio->Divide(ref);
        ratio->Draw("epsame");
        if(leg) {
            auto model = new TF1(Form("modelR%02d_%s", int(r*10.), et.data()), "pol0", 80., 200);
            ratio->Fit(model, "N", "", 80., 200.);
            model->SetLineColor(ratio->GetLineColor());
            model->SetLineStyle(2);
            model->Draw("lsame");
            leg->AddEntry(model, Form("%s: %.2f #pm %.2f", et.data(), model->GetParameter(0), model->GetParError(0)), "l");
        }
    }
    gPad->Update();
}

void makeComparisonRawLevelNew_SpectrumTask(const std::string_view corrfile = "correctedSVD_poor_tc200.root") {
    auto comparisonplot = new ROOT6tools::TSavableCanvas("comparisonTriggersRaw", "Comparison triggers", 1500, 700);
    comparisonplot->Divide(5,2);

    std::unique_ptr<TFile> reader(TFile::Open(corrfile.data(), "READ"));
    std::array<std::string, 3> triggers = {{"INT7", "EJ1", "EJ2"}};
    std::map<std::string, Style> styles = {{"INT7", {kBlack, 20}},{"EJ1", {kRed, 24}}, {"EJ2", {kBlue, 25}}};
    int icol = 1;
    for(auto r : ROOT::TSeqI(2,7)){
        reader->cd(Form("R%02d/rawlevel", r));
        std::map<std::string, TH1 *> triggerhists;
        for(const auto &t : triggers) {
            auto spectrum = gDirectory->Get<TH1>(Form("RawJetSpectrum_FullJets_R%02d_%s_corrected", r, t.data()));
            spectrum->SetName(Form("%scorrected_R%02d", t.data(), r));
            spectrum->Scale(1., "width");
            auto trgstyle = styles[t];
            trgstyle.SetStyle<TH1>(*spectrum);
            spectrum->SetDirectory(nullptr);
            triggerhists[t] = spectrum;
        }

        comparisonplot->cd(icol);
        DrawSpectra(triggerhists, double(r)/10., icol == 1, false);

        comparisonplot->cd(icol+5);
        DrawRatios(triggerhists, double(r)/10., false);
        icol++;
    }
    comparisonplot->cd();
    comparisonplot->Update();
    comparisonplot->SaveCanvas(comparisonplot->GetName());
}