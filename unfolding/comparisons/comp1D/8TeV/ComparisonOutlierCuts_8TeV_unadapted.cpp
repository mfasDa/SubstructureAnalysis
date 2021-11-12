#include "../../../helpers/graphics.C"
#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/Restrictor.cxx"
#include "../../../struct/Ratio.cxx"

void ComparisonOutlierCuts_8TeV() {
    std::vector<std::string> outliercuts = {"outlier1", "outlier15", "outlier2", "outlier25", "outlier3"};
    std::map<std::string, JetSpectrumReader> data;
    std::vector<std::string> spectra = {"normalized_reg4"};
    for(const auto &cut : outliercuts){
        std::stringstream filename;
        filename << "correctedSVD_fine_lowpt_" << cut << ".root";
        data[cut] = JetSpectrumReader(filename.str(), spectra);
    }
    auto jetradii = data.find("outlier3")->second.GetJetSpectra().GetJetRadii();

    std::map<std::string, Style> styles = {
        {"outlier1", {kBlue, 24}},
        {"outlier15", {kRed, 25}},
        {"outlier2", {kGreen, 26}},
        {"outlier25", {kMagenta, 27}},
        {"outlier3", {kBlack, 28}}
    };

    auto plot = new ROOT6tools::TSavableCanvas("comparison_outliercut", "Comparison outliercuts", 300 * jetradii.size(), 700);
    plot->Divide(jetradii.size(), 2);
    Restrictor reported(10, 240);

    int icol = 0;
    for(auto r : jetradii) {
        plot->cd(icol+1);
        GraphicsPad specpad(gPad);
        gPad->SetLogy();
        specpad.Margins(0.15, 0.05, -.1, 0.05);
        specpad.Frame(Form("specpadR%02d", int(r*10.)), "p_{t} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., 250., 1e-10, 1.);
        specpad.FrameTextSize(0.045);
        specpad.Label(0.2, 0.15, 0.45, 0.22, Form("R=%.1f", r));
        if(!icol) specpad.Legend(0.5, 0.65, 0.94, 0.94);

        std::vector<std::pair<Style, Ratio *>> ratios;
        auto refspec = reported(data.find("outlier3")->second.GetJetSpectrum(r, spectra[0]));
        for(auto [var, speccont] : data) {
            if(var == "outlier3") continue;
            auto spec = reported(speccont.GetJetSpectrum(r, spectra[0]));
            double outliercut = double(std::stoi(var.substr(7)));
            if(outliercut > 10) outliercut /= 10.;
            specpad.Draw<TH1>(spec, styles.find(var)->second, Form("outliercut = %.1f * p_{t,hard}", outliercut));
            ratios.push_back({styles.find(var)->second, new Ratio(spec, refspec)});
        }
        specpad.Draw<TH1>(refspec, styles.find("outlier3")->second, "outliercut = 3.0 * p_{t,hard}");

        plot->cd(icol+1+jetradii.size());
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.15, 0.05, -.1, 0.05);
        ratiopad.Frame(Form("specpadR%02d", int(r*10.)), "p_{t} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., 250., 0.7, 1.3);
        ratiopad.FrameTextSize(0.045);
        for(auto [cutstyle, cutratio] : ratios) ratiopad.Draw<Ratio>(cutratio, cutstyle);
        icol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}
