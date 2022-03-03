#include "../../helpers/graphics.C"

void makePlotJetSpectraRatiosWIP(const std::string_view jetspectrafile = "jetspectrumratios.root"){
    std::unique_ptr<TFile> jetspectreader(TFile::Open(jetspectrafile.data(), "READ"));
    auto plot = new ROOT6tools::TSavableCanvas("crossSectionRatios13TeVprelim", "Cross section ratios 13 TeV", 800, 700);
    plot->cd();
    gPad->SetTicks(1,1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);    
    gPad->SetTopMargin(0.05);
    auto frame = new ROOT6tools::TAxisFrame("ratioframe", "#it{p}_{T} (GeV/#it{c})", "#frac{d^{2}#sigma^{#it{R}=0.2}}{d#it{p}_{T}d#eta} / #frac{d^{2}#sigma^{#it{R}=X}}{d#it{p}_{T}d#eta}", 0., 350., 0., 1.4);
    frame->Draw("axis");
    auto prelimlabel = new ROOT6tools::TNDCLabel(0.19, 0.72, 0.74, 0.93, "work in progress");
    prelimlabel->AddText("pp, #sqrt{#it{s}} = 13 TeV, #it{L}_{int} = 6.8 pb^{-1}");
    prelimlabel->AddText("Jets, Anti-#it{k}_{T}");
    prelimlabel->AddText("#it{p}_{T}^{track} > 0.15 GeV/#it{c}, #it{E}^{cluster} > 0.3 GeV");
    prelimlabel->AddText("|#eta^{track}| < 0.7, |#eta^{cluster}| < 0.7, |#eta^{jet}| < 0.7 - #it{R}");
    prelimlabel->SetTextAlign(12);
    prelimlabel->Draw();
    auto rlegend = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.94, 0.4);
    rlegend->Draw();
    auto errlegend = new ROOT6tools::TDefaultLegend(0.15, 0.60, 0.57, 0.7);
    errlegend->Draw();
    
    auto line = new TLine(0., 1., 350., 1.);
    line->SetLineStyle(2);
    line->Draw();
    
    std::map<int, Color_t> specColors = {{2, kBlack}, {3, kRed}, {4, kBlue}, {5, kGreen + 2}, {6, kViolet}};
    std::map<int, Style_t> specstyles = {{2, 20}, {3,21}, {4,22}, {5,23},{6,24}};
    std::map<int, Color_t> shapeColors = {{2, kGray+2}, {3, kRed-9}, {4, kBlue-9}, {5, kGreen}, {6, kViolet-2}};

    for(auto R : ROOT::TSeqI(3, 7)) {
        std::string rstring = Form("R02R%02d", R),
                    rlabel = Form("#it{R}=0.2/#it{R}=%.1f", double(R)/10.);
        jetspectreader->cd(rstring.data());
        auto spec = static_cast<TH1 *>(gDirectory->Get(Form("stat_%s", rstring.data())));
        spec->SetDirectory(nullptr);
        auto corrUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("correlatedUncertainty_%s", rstring.data()))),
             shapeUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("shapeUncertainty_%s", rstring.data())));
        Style{specColors[R], specstyles[R]}.SetStyle<TH1>(*spec);
        spec->Draw("ex0psame");
        rlegend->AddEntry(spec, rlabel.data(), "lep");
        corrUncertainty->SetLineColor(specColors[R]);
        corrUncertainty->SetFillStyle(0);
        corrUncertainty->Draw("2same");
        if(R == 3) errlegend->AddEntry(corrUncertainty, "Correlated uncertainties", "f");
        shapeUncertainty->SetFillColor(shapeColors[R]);
        shapeUncertainty->SetFillStyle(3001);
        shapeUncertainty->Draw("2same");
        if(R == 3) errlegend->AddEntry(shapeUncertainty, "Shape uncertainties", "f");
    }
    plot->SaveCanvas(plot->GetName());
}