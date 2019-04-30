#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/graphics.C"

#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/Ratio.cxx"

void ComparisonRegularizationSVD_SpectrumTask(const std::string_view svdfile){
    std::vector<std::string> spectraSVD;
    for(auto ireg : ROOT::TSeqI(3, 6)) spectraSVD.push_back(Form("normalized_reg%d", ireg));
    auto svddata = JetSpectrumReader(svdfile, spectraSVD);
    auto jetradii = svddata.GetJetSpectra().GetJetRadii();
    int nrad = jetradii.size();

    auto plot = new ROOT6tools::TSavableCanvas("comparisonRegularizationSvd", "Comparison regularization SVD", 300 * nrad, 700);
    plot->Divide(nrad, 2);
    Style style3{kRed, 24}, style5{kBlue, 25}, style4{kBlack, 20};

    int currentcol = 0;
    for(auto r : jetradii) {
        plot->cd(1+currentcol);
        GraphicsPad specpad(gPad);
        gPad->SetLogy();
        specpad.Margins(0.15, 0.04, -1., 0.04);
        specpad.Frame(Form("specframeR%02d", int(r * 10.)), "p_{t} (GeV/c)", "d#sigma/(dp_{t}dy) (mb/(GeV/c))", 0., 350, 1e-9, 100);
        specpad.FrameTextSize(0.045);
        specpad.Label(0.25, 0.15, 0.45, 0.22, Form("R = %.1f", r));
        if(!currentcol) specpad.Legend(0.45, 0.65, 0.94, 0.94);

        auto specSVDreg3 = svddata.GetJetSpectrum(r, "normalized_reg3"),
             specSVDreg4 = svddata.GetJetSpectrum(r, "normalized_reg4"),
             specSVDreg5 = svddata.GetJetSpectrum(r, "normalized_reg5");
        specpad.Draw<TH1>(specSVDreg3, style3, "reg=3");
        specpad.Draw<TH1>(specSVDreg4, style4, "reg=4 (default)");
        specpad.Draw<TH1>(specSVDreg5, style5, "reg=5");

        plot->cd(1+currentcol+nrad);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.15, 0.04, -1., 0.04);
        ratiopad.Frame(Form("ratframeR%02d", int(r * 10.)), "p_{t} (GeV/c)", "reg=x/ reg=4", 0., 350, 0.5, 1.5);
        ratiopad.FrameTextSize(0.045);
        auto methodratio3 = new Ratio(specSVDreg3, specSVDreg4),
             methodratio5 = new Ratio(specSVDreg5, specSVDreg4);
        ratiopad.Draw<Ratio>(methodratio3, style3);
        ratiopad.Draw<Ratio>(methodratio5, style5);

        currentcol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}