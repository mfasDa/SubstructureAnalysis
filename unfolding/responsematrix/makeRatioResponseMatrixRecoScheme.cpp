#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/ResponseReader.cxx"

void makeRatioResponseMatrixRecoScheme(double ultraoption, const std::string_view sysvar){
    std::stringstream filebase, plotname;
    filebase << "correctedSVD";
    plotname << "ratioResponseMatrixRecoScheme";
    if(ultraoption > 0) {
        filebase << "_ultra" << ultraoption;
        plotname << "_ultra" << ultraoption;
    } else if(ultraoption < 0) {
        filebase << "_fine_lowpt";
        plotname << "_fine_lowpt";
    } else {
        filebase << "_lowpt";
        plotname << "_lowpt";
    }
    if(sysvar.length()) {
        filebase << "_" << sysvar;
        plotname << "_" << sysvar;
    }
    filebase << ".root";
    ResponseReader eschemereader(Form("Escheme/%s", filebase.str().data())), 
                   ptschemereader(Form("ptscheme/%s", filebase.str().data()));
    auto jetradii = eschemereader.getJetRadii();

    auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), "Ratio response matrices recombination schemes", 1200, 1000);
    plot->DivideSquare(jetradii.size());

    int ipad(1);
    for(auto r : jetradii) {
        auto matrixescheme = eschemereader.GetResponseMatrixTruncated(r),
             matrixptscheme = ptschemereader.GetResponseMatrixTruncated(r);
        matrixptscheme->Divide(matrixescheme);

        plot->cd(ipad++);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("ratioframe_R%02d", int(r*10)), "p_{t,det} (GeV/c)", "p_{t,part} (GeV/c)", 0., 250, 0., 500.);
        ratiopad.Label(0.25, 0.9, 0.75, 0.98, Form("p_{t}-scheme/E-scheme, R=%.1f", r));
        matrixptscheme->Draw("colzsame");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}