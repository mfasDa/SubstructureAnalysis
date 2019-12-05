#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../struct/JetSpectrumReader.cxx"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/Ratio.cxx"

void ComparisonFullNeutral(int reg = 4){
    std::string nameneutral = Form("normalized_reg%d", reg);
    JetSpectrumReader fullreader("ref/correctedSVD_poor_tc200.root", {"normalized_reg6"}),
                      neutralreader("correctedBayes_poor_default.root", {nameneutral.data()});
    auto jetradii = fullreader.GetJetSpectra().GetJetRadii();
    
    auto plot = new ROOT6tools::TSavableCanvas("comparisonFullNeutral", "comparison full / neutral jets", jetradii.size() * 300, 700);
    plot->Divide(jetradii.size(), 2);

    Style fullstyle{kRed, 24}, neutralstyle{kBlue, 25}, ratiostyle{kBlack, 20};
    
    int icol = 0;
    for(auto R : jetradii){
        plot->cd(icol+1);
        std::vector<double> legrange;
        if(icol == 0) legrange  = {0.35, 0.7, 0.89, 0.89};
        SpectrumPad specpad(gPad, R, "d^{2}#sigma/dp_{t}d#eta (mb/(GeV/c))", {0., 350., 1e-10, 100}, legrange);
        auto fullspec = fullreader.GetJetSpectrum(R, "normalized_reg6"),
             neutralspec = neutralreader.GetJetSpectrum(R, nameneutral);
        specpad.Draw<TH1>(fullspec, fullstyle, "full jets");
        specpad.Draw<TH1>(neutralspec, neutralstyle, "neutral jets");
        plot->cd(icol+1+jetradii.size());
        RatioPad ratiopad(gPad, R, "neutral/full", {0., 350., 0.5, 1.5});
        auto specratio = new Ratio(neutralspec, fullspec);
        ratiopad.Draw<Ratio>(specratio, ratiostyle);
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}