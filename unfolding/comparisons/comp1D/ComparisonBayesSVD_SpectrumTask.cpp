#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/graphics.C"

#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/Ratio.cxx"

void ComparisonBayesSVD_SpectrumTask(const std::string_view svdfile, const std::string_view bayesfile){
    const int regSVD = 6, regBayes = 4;
    std::vector<std::string> spectraBayes = {Form("normalized_reg%d", regBayes)}, spectraSVD =  {Form("normalized_reg%d", regSVD)};
    auto svddata = JetSpectrumReader(svdfile, spectraSVD),
         bayesdata = JetSpectrumReader(bayesfile, spectraBayes);
    auto jetradii = svddata.GetJetSpectra().GetJetRadii();
    int nrad = jetradii.size();

    auto plot = new ROOT6tools::TSavableCanvas("comparisonBayesSvd", "Comparison Bayes - SVD", 300 * nrad, 700);
    plot->Divide(nrad, 2);
    Style svdstyle{kRed, 24}, bayesstyle{kBlue, 25}, ratiostyle{kBlack, 20};

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

        auto specSVD = svddata.GetJetSpectrum(r, spectraSVD[0]),
             specBayes = bayesdata.GetJetSpectrum(r, spectraBayes[0]);
        specpad.Draw<TH1>(specSVD, svdstyle, Form("SVD, reg=%d", regSVD));
        specpad.Draw<TH1>(specBayes, bayesstyle, Form("Bayes, reg=%d", regBayes));

        plot->cd(1+currentcol+nrad);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.15, 0.04, -1., 0.04);
        ratiopad.Frame(Form("ratframeR%02d", int(r * 10.)), "p_{t} (GeV/c)", "Bayes / SVD", 0., 350, 0.5, 1.5);
        ratiopad.FrameTextSize(0.045);
        auto methodratio = new Ratio(specBayes, specSVD);
        ratiopad.Draw<Ratio>(methodratio, ratiostyle);

        currentcol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}