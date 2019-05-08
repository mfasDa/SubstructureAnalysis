#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"

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

void ComparisonRegularization_SpectrumTask(const std::string_view inputfile){
    std::vector<std::string> spectra;
    for(auto ireg : ROOT::TSeqI(1, 10)) spectra.push_back(Form("normalized_reg%d", ireg));
    auto svddata = JetSpectrumReader(inputfile, spectra);
    auto jetradii = svddata.GetJetSpectra().GetJetRadii();
    int nrad = jetradii.size();

    bool isSVD = (inputfile.find("SVD") != std::string::npos);

    std::stringstream plotname;
    auto sysvar = getSysvar(inputfile);
    plotname << "comparisonRegularization" << (isSVD ? "Svd" : "Bayes");
    plotname << "_" << sysvar;
    auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), "Comparison regularization", 300 * nrad, 700);
    plot->Divide(nrad, 2);

    std::array<Color_t, 10> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kMagenta, kGray, kAzure, kCyan};
    std::array<Style_t, 10> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

    int currentcol = 0;
    for(auto r : jetradii) {
        auto ref = svddata.GetJetSpectrum(r, "normalized_reg4");
        plot->cd(1+currentcol);
        GraphicsPad specpad(gPad);
        gPad->SetLogy();
        specpad.Margins(0.15, 0.04, -1., 0.04);
        specpad.Frame(Form("specframeR%02d", int(r * 10.)), "p_{t} (GeV/c)", "d#sigma/(dp_{t}dy) (mb/(GeV/c))", 0., 350, 1e-9, 100);
        specpad.FrameTextSize(0.045);
        specpad.Label(0.25, 0.15, 0.45, 0.22, Form("R = %.1f", r));
        if(!currentcol) specpad.Legend(0.45, 0.35, 0.94, 0.94);

        std::vector<std::pair<Ratio *, Style>> ratios;
        for(auto reg : ROOT::TSeqI(1, 10)){
            TH1 *spec(nullptr);
            Ratio *regratio(nullptr);
            std::stringstream legtitle;
            legtitle << "reg=" << reg;
            if(reg == 4) {
                spec = ref;
                legtitle << " (default)";
            } else {
                spec = svddata.GetJetSpectrum(r, Form("normalized_reg%d", reg));
                regratio = new Ratio(spec, ref);

            }
            Style style{colors[reg], markers[reg]};
            specpad.Draw<TH1>(spec, style, legtitle.str().data());
            if(regratio) ratios.push_back({regratio, style});
        }

        plot->cd(1+currentcol+nrad);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.15, 0.04, -1., 0.04);
        ratiopad.Frame(Form("ratframeR%02d", int(r * 10.)), "p_{t} (GeV/c)", "reg=x/ reg=4", 0., 350, 0.5, 1.5);
        ratiopad.FrameTextSize(0.045);
        for(auto [regratio, style] : ratios) ratiopad.Draw<Ratio>(regratio, style);
        currentcol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}