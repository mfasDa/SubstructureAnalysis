#include "../../helpers/graphics.C"
#include "../../struct/Restrictor.cxx"

void makePlotJetSpectraRatiosPOWHEGPrelim(const std::string_view jetspectrafile = "jetspectrumratios.root", const std::string_view powhegfile = "POWHEGv1.root"){
    std::unique_ptr<TFile> jetspectreader(TFile::Open(jetspectrafile.data(), "READ")),
                           powhegreader(TFile::Open(powhegfile.data(), "READ"));
    Restrictor powhegrange(20, 280);
    auto plot = new ROOT6tools::TSavableCanvas("crossSectionRatios13TeVPOWHEGprelim", "Cross section ratios 13 TeV", 800, 700);
    plot->cd();
    gPad->SetTicks(1,1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);    
    gPad->SetTopMargin(0.05);
    auto frame = new ROOT6tools::TAxisFrame("ratioframe", "#it{p}_{T} (GeV/c)", "#frac{d#sigma^{#it{R}=0.2}}{d#it{p}_{T}d#eta} / #frac{d#sigma^{#it{R}=X}}{d#it{p}_{T}d#eta}", 0., 350., 0., 1.4);
    frame->Draw("axis");
    auto prelimlabel = new ROOT6tools::TNDCLabel(0.19, 0.73, 0.74, 0.94, "ALICE preliminary");
    prelimlabel->AddText("pp, #sqrt{s} = 13 TeV, #it{L}_{int} = 4 pb^{-1}");
    prelimlabel->AddText("Jets, Anti-#it{k}_{T}");
    prelimlabel->AddText("#it{p}_{T}^{ch} > 0.15 GeV/c, #it{E}^{cluster} > 0.3 GeV");
    prelimlabel->AddText("|#eta^{tr}| < 0.7, |#eta^{cluster}| < 0.7, |#eta^{jet}| < 0.7 - #it{R}");
    prelimlabel->SetTextAlign(12);
    prelimlabel->Draw();
    auto rlegend = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.94, 0.4);
    rlegend->Draw();
    auto errlegend = new ROOT6tools::TDefaultLegend(0.15, 0.60, 0.57, 0.7);
    errlegend->Draw();
    auto powheglegend = new ROOT6tools::TDefaultLegend(0.25, 0.2, 0.62, 0.28);
    powheglegend->Draw();
    
    auto line = new TLine(0., 1., 350., 1.);
    line->SetLineStyle(2);
    line->Draw();
    
    std::map<int, Color_t> specColors = {{2, kBlack}, {3, kRed}, {4, kBlue}, {5, kGreen + 2}, {6, kViolet}};
    std::map<int, Style_t> specstyles = {{2, 20}, {3,21}, {4,22}, {5,23},{6,24}};
    std::map<int, Style_t> powhegstyles = {{2, 24}, {3,25}, {4,26}, {5,27},{6,28}};

    std::map<int, Color_t> shapeColors = {{2, kGray+2}, {3, kRed-9}, {4, kBlue-9}, {5, kGreen}, {6, kViolet-2}};

    std::unique_ptr<TH1> powhegref;
    TH1 *spectemplate(nullptr);

    for(auto R : ROOT::TSeqI(3, 6)) {
        std::string rstring = Form("R02R%02d", R),
                    rlabel = Form("#it{R}=0.2/#it{R}=%.1f", double(R)/10.);
        jetspectreader->cd(rstring.data());
        auto spec = static_cast<TH1 *>(gDirectory->Get(Form("stat_%s", rstring.data())));
        spec->SetDirectory(nullptr);
        if(!spectemplate){
            spectemplate = powhegrange(spec);
            spectemplate->SetDirectory(nullptr);
        }
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

        if(!powhegref.get()){
            powhegref = std::unique_ptr<TH1>(static_cast<TH1 *>(powhegreader->Get("hFullJetR02"))->Rebin(spectemplate->GetXaxis()->GetNbins(), "POWHEGREF", spectemplate->GetXaxis()->GetXbins()->GetArray()));
            powhegref->SetDirectory(nullptr);
        }
        std::unique_ptr<TH1> powhegdemom(static_cast<TH1 *>(powhegreader->Get(Form("hFullJetR%02d", R)))->Rebin(spectemplate->GetXaxis()->GetNbins(), "POWHEGDENOM", spectemplate->GetXaxis()->GetXbins()->GetArray()));
        auto powhegratio = static_cast<TH1 *>(powhegref->Clone(Form("POWHEGRatioR02R%02d", R)));
        powhegratio->SetDirectory(nullptr); 
        powhegratio->Divide(powhegdemom.get());
        powhegratio->SetLineColor(specColors[R]);
        powhegratio->SetLineWidth(2);
        powhegratio->Draw("hist L ][ same");
        if(R==4) powheglegend->AddEntry(powhegratio, "POWHEG + PYTHIA8", "l");
    }
    plot->SaveCanvas(plot->GetName());
}