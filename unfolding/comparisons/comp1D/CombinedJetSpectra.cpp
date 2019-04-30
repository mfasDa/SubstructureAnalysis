#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"
#include "../../../helpers/graphics.C"
#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/Ratio.cxx"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/Restrictor.cxx"

void CombinedJetSpectra(const std::string_view inputfile){
    std::vector<std::string> spectra = {"normalized_reg4"};
    JetSpectrumReader reader(inputfile, spectra);
    Restrictor reported(20., 320.);

    std::map<double, Style> styles = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, {0.4, {kOrange, 27}}, {0.5, {kMagenta, 28}}, {0.6, {kGreen, 29}}};

    auto plot = new ROOT6tools::TSavableCanvas("JetSpectra", "Jet Spectra", 700, 800);
    plot->SetLogy();
    GraphicsPad specpad(plot);
    specpad.Margins(0.16, 0.04, -1., 0.04);
    specpad.Frame("specframe", "p_{t} (GeV/c)", "d#sigma/(dp_{t} d#eta) (mb/(GeV/c))", 0., 350, 5e-10, 10);
    specpad.FrameTextSize(0.045);
    specpad.Label(0.2, 0.15, 0.8, 0.22, "pp, #sqrt{s} = 13 TeV, Full jets, anti-k_{t}");
    specpad.Legend(0.6, 0.5, 0.89, 0.89);
    int ipower = 0;
    for(auto r : reader.GetJetSpectra().GetJetRadii()) {
        auto spec = reported(reader.GetJetSpectrum(r, "normalized_reg4"));
        auto norm = TMath::Power(2, ipower++);
        spec->Scale(norm);
        specpad.Draw<TH1>(spec, styles.find(r)->second, Form("R=%.1f x %d", r, int(norm)));
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}