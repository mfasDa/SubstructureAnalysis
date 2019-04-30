#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"
#include "../../../helpers/graphics.C"
#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/Ratio.cxx"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/Restrictor.cxx"
#include "../../../struct/Ratio.cxx"

void CombinedJetRatios(const std::string_view inputfile){
    std::vector<std::string> spectra = {"normalized_reg4"};
    JetSpectrumReader reader(inputfile, spectra);
    Restrictor reported(20., 320.);

    std::map<double, Style> styles = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, {0.4, {kOrange, 27}}, {0.5, {kMagenta, 28}}, {0.6, {kGreen, 29}}};

    auto plot = new ROOT6tools::TSavableCanvas("JetRatios", "Jet Ratios", 700, 800);
    GraphicsPad specpad(plot);
    specpad.Margins(0.16, 0.04, -1., 0.04);
    specpad.Frame("specframe", "p_{t} (GeV/c)", "d#sigma(R=0.2)/(dp_{t} d#eta) / d#sigma(R=X)/(dp_{t} d#eta)", 0., 350, 0, 1);
    specpad.FrameTextSize(0.045);
    specpad.Label(0.2, 0.84, 0.8, 0.94, "pp, #sqrt{s} = 13 TeV, Full jets, anti-k_{t}");
    specpad.Legend(0.7, 0.15, 0.94, 0.5);
    auto numerator = reported(reader.GetJetSpectrum(0.2, "normalized_reg4"));
    for(auto r : reader.GetJetSpectra().GetJetRadii()) {
        if(r==0.2) continue;
        auto specratio = new Ratio(numerator, reported(reader.GetJetSpectrum(r, "normalized_reg4")));
        specpad.Draw<Ratio>(specratio, styles.find(r)->second, Form("R=%.1f", r));
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}