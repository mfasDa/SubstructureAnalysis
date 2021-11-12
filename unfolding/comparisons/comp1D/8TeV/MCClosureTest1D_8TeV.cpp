#include "../../../../meta/stl.C"
#include "../../../../meta/root.C"
#include "../../../../meta/root6tools.C"

#include "../../../../helpers/filesystem.C"
#include "../../../../helpers/graphics.C"
#include "../../../../helpers/math.C"
#include "../../../../helpers/root.C"
#include "../../../../helpers/string.C"
#include "../../../../struct/JetSpectrumReader.cxx"
#include "../../../../struct/GraphicsPad.cxx"
#include "../../../../struct/Ratio.cxx"

void MCClosureTest1D_8TeV(const std::string_view inputfile, string outputdir, string filetype = "png") {
    std::vector<std::string> spectra = {"partclosure"};
    for(auto i : ROOT::TSeqI(1,11)) spectra.push_back(Form("unfoldedClosure_reg%d", i));
    auto data = JetSpectrumReader(inputfile, spectra);
    auto jetradii = data.GetJetSpectra().GetJetRadii();
    bool isSVD = (inputfile.find("SVD") != std::string::npos);

    auto plot = new TCanvas(Form("MCClosureTest1D%s", (isSVD ? "Svd" : "Bayes")), Form("Monte-Calro closure test (%s unfolding)", (isSVD ? "SVD" : "Bayes")), jetradii.size() * 300., 700.);
    plot->Divide(jetradii.size(), 2);

    std::array<Color_t, 10> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kMagenta, kGray, kAzure, kCyan};
    std::array<Style_t, 10> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};
    Style rawstyle{kBlack, 20};

    int currentcol = 0;
    for(auto rvalue : jetradii){
        std::string rstring(Form("R%02d", int(rvalue*10.)));

        auto *htruth = data.GetJetSpectrum(rvalue, "partclosure");
        htruth->Scale(1., "width");

        plot->cd(1+currentcol);
        gPad->SetLogy();
        GraphicsPad specpad(gPad);
        specpad.Margins(0.17, 0.04, -1., 0.04);
        specpad.Frame(Form("specframe_%s", rstring.data()), "p_{t} (GeV/c)", "d#sigma/dp_{t} (mb/(GeV/c))", 0., 350., 1e-10, 10);
        specpad.FrameTextSize(0.045);
        specpad.Label(0.25, 0.15, 0.45, 0.22, Form("R = %.1f", rvalue));
        if(!currentcol) specpad.Legend(0.65, 0.45, 0.94, 0.94);
        specpad.Draw<TH1>(htruth, rawstyle, "true");

        plot->cd(1 + currentcol + jetradii.size());
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.17, 0.04, -1., 0.04);
        ratiopad.Frame(Form("ratioframe_%s", rstring.data()), "p_{t} (GeV/c)", "Unfolded/true", 0., 350., 0.5, 1.5);
        ratiopad.FrameTextSize(0.045);

        for(auto ireg : ROOT::TSeqI(1, 11)){
            auto unfolded = data.GetJetSpectrum(rvalue, Form("unfoldedClosure_reg%d", ireg));
            Style varstyle{colors[ireg-1], markers[ireg-1]};
            specpad.Draw<>(unfolded, varstyle, Form("reg=%d", ireg));
            auto ratiotrue = new Ratio(unfolded, htruth);
            ratiopad.Draw<Ratio>(ratiotrue, varstyle);
        }
        currentcol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveAs(Form("%s/%s.%s", outputdir.c_str(), plot->GetName(), filetype.c_str()));
}