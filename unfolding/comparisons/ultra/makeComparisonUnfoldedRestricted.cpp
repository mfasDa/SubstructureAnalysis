#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"
#include "../../binnings/binningPt1D.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/Restrictor.cxx"
#include "../../../struct/Ratio.cxx"

void makeComparisonUnfoldedRestricted(const std::string_view sysvar){
    std::vector<std::string> spectra = {"normalized_reg4"};
    auto dataUltra = JetSpectrumReader(Form("correctedSVD_ultra240_%s.root", sysvar.data()), spectra), 
         dataReg = JetSpectrumReader(Form("correctedSVD_fine_lowpt_%s.root", sysvar.data()), spectra);
    auto jetradii = dataUltra.GetJetSpectra().GetJetRadii();
    Restrictor reported(20., 240.);
    
    auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonUnfoldedRanges%s", sysvar.data()), "Comparison raw spectra in identical range", 300 * jetradii.size(), 700);
    plot->Divide(jetradii.size(),2);

    Style ultrastyle{kRed, 24}, regstyle{kBlue, 25}, ratiostyle{kBlack, 20};
    int icol = 0;
    for(auto r : jetradii){
        plot->cd(icol+1);
        gPad->SetLogy();
        GraphicsPad specpad(gPad);
        specpad.Margins(0.17, 0.04, -1., 0.02);
        specpad.Frame(Form("specframeR%02d", int(r * 10.)), "p_{t} (GeV/c)", "d#sigma/dp_{t}d#eta (mb/(GeV/c))", 0., 250, 1e-7, 1e-2);
        specpad.FrameTextSize(0.047);
        specpad.Label(0.25, 0.15, 0.4, 0.22, Form("R=%.1f", r));
        if(!icol) specpad.Legend(0.3, 0.7, 0.93, 0.93);
        auto specultra = reported(dataUltra.GetJetSpectrum(r, spectra[0])),
             specreg = reported(dataReg.GetJetSpectrum(r, spectra[0]));
        specpad.Draw<TH1>(specreg, regstyle, "10 GeV/c < p_{t,j,det} < 200 GeV/c");
        specpad.Draw<TH1>(specultra, ultrastyle, "10 GeV/c < p_{t,j,det} < 240 GeV/c");

        plot->cd(icol+jetradii.size()+1);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.17, 0.04, -1., 0.02);
        ratiopad.Frame(Form("specframeR%02d", int(r * 10.)), "p_{t} (GeV/c)", "large range /vormal range", 0., 250, 0.8, 1.2);
        ratiopad.FrameTextSize(0.047);
        Ratio *specratio = new Ratio(specultra,specreg);
        ratiopad.Draw<Ratio>(specratio, ratiostyle);
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}