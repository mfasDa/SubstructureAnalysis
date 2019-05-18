#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../struct/Restrictor.cxx"

TH1 *makeRelStat(const TH1 *in) {
    auto result = static_cast<TH1*>(in->Clone(Form("rel%s", in->GetName())));
    result->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
        auto val = result->GetBinContent(b+1);
        result->SetBinContent(b+1, 1);
        result->SetBinError(b+1, result->GetBinError(b+1)/val);
    }
    return result;
}

TGraphAsymmErrors *makeRelSys(const TGraphAsymmErrors *in) {
    auto result = new TGraphAsymmErrors;
    for(auto p : ROOT::TSeqI(0, in->GetN())){
        auto val = in->GetY()[p];
        result->SetPoint(p, in->GetX()[p], 1.);
        result->SetPointError(p, in->GetEXlow()[p], in->GetEXhigh()[p], in->GetEYlow()[p]/val, in->GetEYhigh()[p]/val);
    }
    return result;
}

void makePlotJetSpectrumPOWHEGPrelim(int R = 2, const std::string_view spectrumfile = "jetspectrum.root" , const std::string_view powhegfile = "POWHEGv1.root") {
    Restrictor powhegrange(20., 280);
    std::unique_ptr<TFile> jetspectreader(TFile::Open(spectrumfile.data(), "READ")),
                           powhegreader(TFile::Open(powhegfile.data(), "READ"));
                            
    std::string rstring = Form("R%02d", R),
                rtitle = Form("#it{R}=%.1f", double(R)/10.);
    auto plot = new ROOT6tools::TSavableCanvas(Form("compJetSpectrumALICEPOWHEGprelim%s", rstring.data()), "Comparison ALICE jet spectrum to POWHEG", 700, 1000);
    
    double xmargin = 0.16;
    auto specpanel = new TPad("specpanel", "Spectrum Panel", 0., 0.3, 1.,1.);
    specpanel->Draw();
    specpanel->cd();
    gPad->SetLogy();
    gPad->SetTicks(1,1);
    gPad->SetLeftMargin(xmargin);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0);
    gPad->SetTopMargin(0.05);

    double ysizespec = 0.045;
    auto specframe = new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/#it{c})", "#frac{d#sigma}{d#it{p}_{T}d#eta} (mb/(GeV/#it{c}))", 0., 350., 5e-9, 1);
    specframe->GetYaxis()->SetTitleSize(ysizespec);
    specframe->GetYaxis()->SetLabelSize(ysizespec);
    specframe->Draw("axis");

    auto prelimlabel = new ROOT6tools::TNDCLabel(0.39, 0.72, 0.94, 0.92, "ALICE preliminary");
    prelimlabel->AddText("pp, #sqrt{s} = 13 TeV, #it{L}_{int} = 4 pb^{-1}");
    prelimlabel->AddText(Form("Jets, Anti-#it{k}_{T}, %s", rtitle.data()));
    prelimlabel->AddText("#it{p}_{T}^{track} > 0.15 GeV/#it{c}, #it{E}^{cluster} > 0.3 GeV");
    prelimlabel->AddText(Form("|#eta^{track}| < 0.7, |#eta^{cluster}| < 0.7, |#eta^{jet}| < %.1f", 0.7 - double(R)/10.));
    prelimlabel->SetTextAlign(12);
    prelimlabel->Draw();

    auto speclegend = new ROOT6tools::TDefaultLegend(0.5, 0.55, 0.94, 0.7);
    speclegend->Draw();
    auto errlegend = new ROOT6tools::TDefaultLegend(0.2, 0.04, 0.6, 0.16);
    errlegend->Draw();

    jetspectreader->cd(rstring.data());
    auto spec = static_cast<TH1 *>(gDirectory->Get(Form("stat_%s", rstring.data())));
    spec->SetDirectory(nullptr);
    auto corrUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("correlatedUncertainty_%s", rstring.data()))),
         shapeUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("shapeUncertainty_%s", rstring.data())));
    auto speclimited = powhegrange(spec);
    Style{kBlack, 20}.SetStyle<TH1>(*spec);
    spec->Draw("ex0same");
    speclegend->AddEntry(spec, "ALICE Data", "lep");
    corrUncertainty->SetLineColor(kBlack);
    corrUncertainty->SetFillStyle(0);
    corrUncertainty->Draw("2same");
    errlegend->AddEntry(corrUncertainty, "Correlated uncertainties", "f");
    shapeUncertainty->SetFillColor(kGray);
    shapeUncertainty->SetFillStyle(3001);
    shapeUncertainty->Draw("2same");
    errlegend->AddEntry(shapeUncertainty, "Shape uncertainties", "f");

    auto powhegspec = static_cast<TH1 *>(powhegreader->Get(Form("hFullJet%s", rstring.data())))->Rebin(speclimited->GetXaxis()->GetNbins(), Form("powhegrebinned%s", rstring.data()), speclimited->GetXaxis()->GetXbins()->GetArray());
    powhegspec->SetDirectory(nullptr);
    powhegspec->Scale(1., "width");
    Style{kBlue, 24}.SetStyle<TH1>(*powhegspec);
    powhegspec->Draw("epsame");
    speclegend->AddEntry(powhegspec, "POWHEG+PYTHIA8", "lep");

    plot->cd();

    auto ratiopanel = new TPad("ratiopanel", "Spectrum Panel", 0., 0.0, 1.,0.3);
    ratiopanel->Draw();
    ratiopanel->cd();
    gPad->SetTicks(1,1);
    gPad->SetLeftMargin(xmargin);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.2);
    gPad->SetTopMargin(0.);

    double ysizeratio = 0.085;
    auto ratioframe = new ROOT6tools::TAxisFrame("specframe", "#it{p}_{T} (GeV/c)", "MC / Data", 0., 350., 0.45, 1.55);
    ratioframe->GetXaxis()->SetTitleSize(ysizeratio);
    ratioframe->GetXaxis()->SetLabelSize(ysizeratio); 
    ratioframe->GetYaxis()->SetTitleSize(ysizeratio);
    ratioframe->GetYaxis()->SetLabelSize(ysizeratio); 
    ratioframe->GetYaxis()->SetTitleOffset(0.8);
    ratioframe->Draw();

    auto statrel = makeRelStat(spec);
    Style{kBlack, 20}.SetStyle<TH1>(*statrel);
    auto corrrel = makeRelSys(corrUncertainty), shaperel = makeRelSys(shapeUncertainty);
    corrrel->SetLineColor(kBlack);
    corrrel->SetFillStyle(0);
    corrrel->Draw("2same");
    shaperel->SetFillColor(kGray);
    shaperel->SetFillStyle(3001);
    shaperel->Draw("2same");

    auto ratioPOWHEGdata = static_cast<TH1*>(powhegspec->Clone("RatioPowhegData"));
    ratioPOWHEGdata->SetDirectory(nullptr);
    ratioPOWHEGdata->Divide(speclimited);
    Style{kBlue, 24}.SetStyle<TH1>(*ratioPOWHEGdata);
    ratioPOWHEGdata->Draw("epsame");

    auto line = new TLine(0., 1., 350., 1.);
    line->SetLineStyle(2);
    line->Draw();
    auto box = new TBox(0., 0.95, 5., 1.05);
    box->SetFillColor(kOrange);
    box->SetLineWidth(0);
    box->Draw();

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}