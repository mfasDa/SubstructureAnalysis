#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"
#include "../../../helpers/root.C"
#include "../../../helpers/string.C"
#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/Ratio.cxx"

std::string getSysvar(const std::string_view inputfile) {
    auto filebase = basename(inputfile);
    std::string sysvar;
    if(filebase.find("_") != std::string::npos){
        int sysstart = filebase.find_first_of("_") + 1,
            sysend = filebase.find_last_of(".");
        sysvar = std::string(filebase.substr(sysstart, sysend - sysstart));
    }
    return sysvar;
}

void ComparisonFoldRaw_SpectrumTask(const std::string_view inputfile, int plotptmax = 250.){
    std::vector<std::string> spectra = {"hraw"};
    auto regmin = 2, regmax = 9;
    for(auto i : ROOT::TSeqI(regmin,regmax+1)) spectra.push_back(Form("backfolded_reg%d", i));
    auto data = JetSpectrumReader(inputfile, spectra);
    auto jetradii = data.GetJetSpectra().GetJetRadii();

    bool isSVD = (inputfile.find("SVD") != std::string::npos);

    std::stringstream plotname;
    auto sysvar = getSysvar(inputfile);
    plotname << "comparisonFoldRaw" << (isSVD ? "Svd" : "Bayes");
    plotname << "_" << sysvar;
    auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), Form("Comparison back-folded raw (%s unfolding)", (isSVD ? "SVD" : "Bayes")), jetradii.size() * 300., 700.);
    plot->Divide(jetradii.size(), 2);

    std::array<Color_t, 10> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kMagenta, kGray, kAzure, kCyan};
    std::array<Style_t, 10> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};
    Style rawstyle{kBlack, 20};

    int currentcol = 0;
    for(auto rvalue : jetradii){
        std::string rstring(Form("R%02d", int(rvalue*10.)));
        auto rawspectrum = data.GetJetSpectrum(rvalue, "hraw");

        plot->cd(1+currentcol);
        gPad->SetLogy();
        GraphicsPad specpad(gPad);
        specpad.Margins(0.17, 0.04, -1., 0.04);
        specpad.Frame(Form("specframe_%s", rstring.data()), "p_{t} (GeV/c)", "1/N_{ev} dN/dp_{t} ((GeV/c)^{-1})", 0., plotptmax, 1e-10, 1);
        specpad.FrameTextSize(0.045);
        specpad.Label(0.25, 0.15, 0.45, 0.22, Form("R = %.1f", rvalue));
        TLegend *leg(nullptr);
        if(!currentcol) specpad.Legend(0.6, 0.45, 0.94, 0.94);
        specpad.Draw<TH1>(rawspectrum, rawstyle, "raw");

        plot->cd(1 + currentcol + jetradii.size());
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.17, 0.04, -1., 0.04);
        ratiopad.Frame(Form("ratioframe_%s", rstring.data()), "p_{t} (GeV/c)", "Folded/raw", 0., plotptmax, 0.5, 1.5);
        ratiopad.FrameTextSize(0.045);

        for(auto ireg : ROOT::TSeqI(regmin,regmax+1)){
            auto backfolded = data.GetJetSpectrum(rvalue, Form("backfolded_reg%d", ireg));
            backfolded->Scale(1., "width");
            Style varstyle{colors[ireg-1], markers[ireg-1]};
            specpad.Draw<TH1>(backfolded, varstyle, Form("reg=%d", ireg));
            auto ratioraw = new Ratio(backfolded, rawspectrum);
            ratiopad.Draw<Ratio>(ratioraw, varstyle);
        }
        currentcol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}