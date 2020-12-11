#include "../../helpers/graphics.C"
void Scale(TH1 *stat, TGraphAsymmErrors *corr, TGraphAsymmErrors *shape, double scale) {
    stat->Scale(scale);
    for(auto b : ROOT::TSeqI(0, corr->GetN())){
        corr->SetPoint(b, corr->GetX()[b], corr->GetY()[b] * scale);
        shape->SetPoint(b, shape->GetX()[b], shape->GetY()[b] * scale);
        corr->SetPointError(b, corr->GetEXlow()[b], corr->GetEXhigh()[b], corr->GetEYlow()[b] * scale, corr->GetEYlow()[b] * scale);
        shape->SetPointError(b, shape->GetEXlow()[b], shape->GetEXhigh()[b], shape->GetEYlow()[b] * scale, shape->GetEYlow()[b] * scale);
    }
}

void makePlotJetSpectraWIP(const std::string_view jetspectrafile = "jetspectrum.root") {
    std::unique_ptr<TFile> jetspectreader(TFile::Open(jetspectrafile.data(), "READ"));

    auto plot = new ROOT6tools::TSavableCanvas("jetSpectrapp13TeVprelim", "Jet spectra 13 TeV", 700, 800);
    plot->SetLogy();
    gPad->SetTicks(1,1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);    
    gPad->SetTopMargin(0.05);
    auto frame = new ROOT6tools::TAxisFrame("specframe", "#it{p}_{T} (GeV/#it{c})", "#frac{d^{2}#sigma}{d#it{p}_{T}d#eta} (mb/(GeV/#it{c}))", 0., 350., 1e-8, 100);
    frame->Draw("axis");

    auto prelimlabel = new ROOT6tools::TNDCLabel(0.39, 0.72, 0.94, 0.93, "ALICE work in progress");
    prelimlabel->AddText("pp, #sqrt{#it{s}} = 13 TeV, #it{L}_{int} = 6.7 pb^{-1}");
    prelimlabel->AddText("Jets, Anti-#it{k}_{T}");
    prelimlabel->AddText("#it{p}_{T}^{track} > 0.15 GeV/#it{c}, #it{E}^{cluster} > 0.3 GeV");
    prelimlabel->AddText("|#eta^{track}| < 0.7, |#eta^{cluster}| < 0.7, |#eta^{jet}| < 0.7 - #it{R}");
    prelimlabel->SetTextAlign(12);
    prelimlabel->Draw();

    auto rlegend = new ROOT6tools::TDefaultLegend(0.65, 0.41, 0.94, 0.73);
    rlegend->Draw();
    auto errlegend = new ROOT6tools::TDefaultLegend(0.2, 0.12, 0.6, 0.24);
    errlegend->Draw();

    std::map<int, Color_t> specColors = {{2, kBlack}, {3, kRed}, {4, kBlue}, {5, kGreen + 2}, {6, kViolet}};
    std::map<int, Style_t> specstyles = {{2, 20}, {3,21}, {4,33}, {5,34},{6,29}};
    std::map<int, Color_t> shapeColors = {{2, kGray+2}, {3, kRed-9}, {4, kBlue-9}, {5, kGreen}, {6, kViolet-2}};
    std::map<int, double> scales = {{3, 3}, {4, 10}, {5, 30}, {6, 100}};
    
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring = Form("R%02d", R),
                    rlabel = Form("#it{R}=%.1f", double(R)/10.);
        jetspectreader->cd(rstring.data());
        auto spec = static_cast<TH1 *>(gDirectory->Get(Form("stat_%s", rstring.data())));
        spec->SetDirectory(nullptr);
        auto corrUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("correlatedUncertainty_%s", rstring.data()))),
             shapeUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("shapeUncertainty_%s", rstring.data())));
        std::stringstream rlegbuilder;
        rlegbuilder << rlabel;
        if(R>2) {
            double myscale = scales[R];
            Scale(spec, corrUncertainty, shapeUncertainty, myscale);
            rlegbuilder << " x " << int(myscale);
        }
        Style{specColors[R], specstyles[R]}.SetStyle<TH1>(*spec);
        if(R>3) spec->SetMarkerSize(1.5);
        spec->Draw("ex0psame");
        rlegend->AddEntry(spec, rlegbuilder.str().data(), "lep");
        corrUncertainty->SetLineColor(specColors[R]);
        corrUncertainty->SetFillStyle(0);
        corrUncertainty->Draw("2same");
        if(R == 2) errlegend->AddEntry(corrUncertainty, "Correlated uncertainties", "f");
        shapeUncertainty->SetFillColor(shapeColors[R]);
        shapeUncertainty->SetFillStyle(3001);
        shapeUncertainty->Draw("2same");
        if(R == 2) errlegend->AddEntry(shapeUncertainty, "Shape uncertainties", "f");
    }
    plot->SaveCanvas(plot->GetName());
}