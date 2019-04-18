#include "../helpers/graphics.C"

std::map<double, TH1 *> readCorrectedSpectra(const std::string_view filename) {
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(auto r = 0.2; r <= 0.6; r += 0.1) {
        std::string rstring = Form("R%02d", int(r*10.));
        reader->cd(rstring.data());
        gDirectory->cd("reg4");
        auto spectrum = static_cast<TH1 *>(gDirectory->Get("normalized_reg4"));
        spectrum->SetDirectory(nullptr);
        result[r] = spectrum;
    }
    return result;
}

void comparisonFullSpectraTRD(std::string_view sysvar = "notc") {
    auto specwith = readCorrectedSpectra(Form("withTRD/correctedSVD_fine_lowpt_%s.root", sysvar.data())),
         specwithout = readCorrectedSpectra(Form("withoutTRD/correctedSVD_fine_lowpt_%s.root", sysvar.data()));

    auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonFullJetsTRD_%s", sysvar.data()), "Comparison full jets with TRD in reconstruction", 300 * specwith.size(), 700);
    plot->Divide(specwith.size(), 2);

    int icol = 0;
    Style withstyle{kRed, 24}, withoutstyle{kBlue, 25}, ratiostyle{kBlack, 20};
    for(auto r = 0.2; r <= 0.6; r += 0.1){
        plot->cd(icol+1);
        gPad->SetLogy();
        gPad->SetRightMargin(0.04);
        gPad->SetLeftMargin(0.17);
        gPad->SetTopMargin(0.04);
        gPad->SetBottomMargin(0.13);
        auto specframe = new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(r*10.)), "p_{t} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., 300., 1e-9, 100.);
        specframe->GetXaxis()->SetTitleSize(0.06);
        specframe->GetYaxis()->SetTitleSize(0.06);
        specframe->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.2, 0.15, 0.7, 0.3, Form("Full jets, R=%.1f", r)))->Draw();
        TLegend *leg(nullptr);
        if(!icol) {
            leg = new ROOT6tools::TDefaultLegend(0.45, 0.6, 0.94, 0.94);
            leg->Draw();
        }
        auto wtrd = specwith.find(r)->second,
             wotrd = specwithout.find(r)->second;
        withstyle.SetStyle<TH1>(*wtrd);
        withoutstyle.SetStyle<TH1>(*wotrd);
        wtrd->Draw("epsame");
        wotrd->Draw("epsame");
        if(leg) {
            leg->AddEntry(wtrd, "with TRD", "lep");
            leg->AddEntry(wotrd, "without TRD", "lep");
        }

        plot->cd(icol + 1 + specwith.size());
        gPad->SetRightMargin(0.04);
        gPad->SetLeftMargin(0.17);
        gPad->SetTopMargin(0.04);
        gPad->SetBottomMargin(0.13);
        auto ratioframe = new ROOT6tools::TAxisFrame(Form("ratioframeR%02d", int(r*10.)), "p_{t} (GeV/c)", "without / with TRD", 0., 300., 0., 2.);
        ratioframe->GetXaxis()->SetTitleSize(0.06);
        ratioframe->GetYaxis()->SetTitleSize(0.06);
        ratioframe->Draw("axis");
        auto ratio = static_cast<TH1 *>(wotrd->Clone(Form("RatioTRD_R%02d", int(r*10.))));
        ratio->Divide(wtrd);
        ratiostyle.SetStyle<TH1>(*ratio);
        ratio->Draw("epsame");
        icol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}