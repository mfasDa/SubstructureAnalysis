#include "../../binnings/binningPt1D.C"
#include "../../../helpers/graphics.C"
#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/Ratio.cxx"
#include "../../../struct/Restrictor.cxx"
#include "../../../struct/JetSpectrumReader.cxx"

struct variation {
    int         fTCcut;
    Style       fStyle;

    bool operator<(const variation &other) const { return fTCcut < other.fTCcut; }
};

std::vector<variation> getVariations(int ultrarange) {
    std::vector<variation> result;
    result.push_back({250, {kBlue, 25}});
    if(ultrarange == 300)result.push_back({1000, {kRed, 24}});
    return result;
}

std::string makeLegEntry(int tccut) {
    std::stringstream legentry;
    legentry << "p_{t}^{t,c} < ";
    if(tccut == 1000) legentry << ultrarange;
    else legentry << tccut;
    legentry << " GeV/c";
    return legentry.str();
}

void makeComparisonUnfoldedTruncationConstituents(int ultrarange) {
    std::vector<std::string> spectra = {"normalized_reg4"};
    auto reference = JetSpectrumReader(Form("correctedSVD_ultra%d_tc200.root", ultrarange), spectra);
    auto jetradii = reference.GetJetSpectrum().GetJetRadii();
    std::map<variation, JetSpectrumReader>> variations;
    for(auto v : getVariations(ultrarange)) {
        std::stringstream filename;
        filename << "correctedSVD_ultra" << ultrarange;
        if(v.fTCcut < 1000) filename << "_tc" << v.fTCcut;
        else filename << "_notc";
        filename << ".root";
        variations[v] = JetSpectrumReader(filename.str().data(), spectra);
    }

    double framemaxy, ptmaxy;
    switch(ultrarange){
        case 240: framemaxy = 350; ptmaxy = 320;
        case 300: framemaxy = 450; ptmaxy = 400;
        default: framemaxy = 250; ptmaxy = 240;
    };
    Restrictor reported(10, ptmaxy);
    Style refstyle{kBlack, 20};

    auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonTruncationClusterTrack_unfolding%d", ultrarange), Form("Comparison unfolded spectra with different cluster / track cuts"), 300 * jetradii.size(), 700);
    plot->Divide(jetradii.size(), 2);
    int icol = 0;
    for(double r = 0.2; r <= 0.6; r += 0.1) {
        plot->cd(icol+1);
        gPad->SetLogy();
        GraphicsPad specpad(gPad);
        specpad.Margins(0.17, 0.04, -1., 0.04);
        std::string rstring = Form("R%02d", int(r*10.));
        specpad.Frame(Form("specframeR%02d", int(r*10.)), "p_{t,jet} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., framemaxy, 1e-9, 1.);
        specpad.FrameTextSize(0.047);
        specpad.Label(0.22, 0.15, 0.47, 0.22, Form("R < %.1f", r));
        if(!icol) {
            specpad.Label(0.25, 0.84, 0.94, 0.94, Form("10 GeV/c < p_{t,det} < %d GeV/c", ultrarange));
            specpad.Legend(0.45, 0.5, 0.89, 0.89);
        }
        auto refspec = reported(reference.GetJetSpectrum(r, spectra[0]));
        specpad.Draw<TH1>(refspec, refstyle, "p_{t}^{t,c} < 200 GeV/c");
        std::map<Ratio *, Style> ratios;
        for(auto v : variations) {
            auto varspec = reported(v.second.GetJetSpectrum(r, spectra[0]));
            specpad.Draw<TH1>(varspec, v.first.Style, makeLegEntry(v.first.fTCcut));
            ratios.emplace_back(new Ratio(varspec, reference));
            ratios.insert(std::pair<Ratio *, Style>(new Ratio(varspec, refspec), v.first.fStyle));
        }

        plot->cd(icol+1+reference.size());        
        GraphicsPad ratiopad;
        ratiopad.Margins(0.17, 0.04, -.1, 0.04);
        ratiopad.Frame(Form("ratioframeR%02d", int(r*10.)), "p_{t,jet} (GeV/c)", "variation / p_{t}^{t,c} < 200 GeV/c", 0., framemaxy, 0.7, 1.3);
        ratiopad.FrameTextSize(0.047);
        for(auto &[ratio, style] : ratios) ratiopad.Draw<TH1>(ratio, style);
        icol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}