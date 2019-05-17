#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/JetSpectrumReader.cxx"
#include "../../struct/Ratio.cxx"
#include "../../helpers/graphics.C"

std::map<int, TH1 *> readSpectra(const std::string_view inputfile) {
    std::vector<std::string> spectra = {"normalized_reg6"};
    JetSpectrumReader reader(inputfile, spectra);
    std::map<int, TH1*> output;
    for(auto R : ROOT::TSeqI(2, 7)) {
        auto spec = reader.GetJetSpectrum(double(R)/10., spectra[0]);
        spec->SetDirectory(nullptr);
        output[R] = spec;
    }
    return output;
}

void compareUnfoldedMaxTrackPt(){
    std::vector<int> trackcuts = {100, 125, 150, 175, 200}; 
    std::map<int, std::map<int, TH1 *>> data;
    for(auto tc : trackcuts) {
        data[tc] = readSpectra(Form("correctedSVD_poor_t%d.root", tc));
    }

    std::map<int, Style> tcstyles = {{100, {kRed, 24}}, {125, {kBlue, 25}}, {150, {kGreen+2, 27}}, {175, {kOrange, 27}}, {200, {kViolet, 28}}}; 

    auto plot = new ROOT6tools::TSavableCanvas("comparisonMaxTrackCut", "Comparison maximum track cut", 1500, 700);
    plot->Divide(5,2);

    std::map<int, TH1 *> refspectra = data.find(100)->second;

    int icol(0);
    for(auto R : ROOT::TSeqI(2, 7)){
        plot->cd(icol+1);
        GraphicsPad specpad(gPad);
        specpad.Logy();
        specpad.Margins(0.15, 0.05, -1., 0.05);
        specpad.Frame(Form("specpadR%02d", R), "p_{t} (GeV/c)", "d#sigma/dp_{t}d#eta (mb/(GeV/c))", 0., 350, 1e-12, 100);
        specpad.FrameTextSize(0.047);
        specpad.Label(0.2, 0.15, 0.45, 0.22, Form("R=%.1f", double(R)/10.));
        if(icol == 0) specpad.Legend(0.45, 0.5, 0.94, 0.94);
        std::vector<std::pair<Style, Ratio *>> ratios;
        for(auto [tc, hists] : data) {
            auto spec = hists.find(R)->second;
            specpad.Draw<TH1>(spec, tcstyles.find(tc)->second, Form("p_{t,track} < %d GeV/c", tc));
            if(tc > 100) {
                auto rat = new Ratio(spec, refspectra.find(R)->second);
                rat->SetDirectory(nullptr);
                ratios.push_back({tcstyles.find(tc)->second, rat});
            }
        }

        plot->cd(icol+6);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.15, 0.05, -1., 0.05);
        ratiopad.Frame(Form("specpadR%02d", R), "p_{t} (GeV/c)", "variation / p_{t,trk} < 100", 0., 350, 0.5, 1.5);
        ratiopad.FrameTextSize(0.047);
        for(auto [sty, rat]: ratios) ratiopad.Draw<Ratio>(rat, sty);
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}