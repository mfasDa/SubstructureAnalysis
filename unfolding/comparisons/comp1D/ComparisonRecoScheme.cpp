#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"
#include "../../../helpers/graphics.C"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/Ratio.cxx"
#include "../../../struct/Restrictor.cxx"

void ComparisonRecoScheme(int ultraoption, const std::string_view sysvar){
    std::stringstream basefile, plotname;
    basefile << "correctedSVD";
    plotname << "comparisonRecoScheme";
    if(ultraoption > 0){
        basefile << "_ultra" << ultraoption;
        plotname << "_ultra" << ultraoption;
    } 
    else if(ultraoption < 0){
        basefile << "_fine_lowpt";
        plotname << "_fine_lowpt";
    }
    else{
        basefile << "_lowpt";
        plotname << "_lowpt";
    }
    if(sysvar.length()){
        basefile << "_" << sysvar;
        plotname << "_" << sysvar;
    }
    basefile << ".root";
    std::vector<std::string> branchlist = {"normalized_reg4"};
    JetSpectrumReader eschemereader(Form("Escheme/%s", basefile.str().data()), branchlist), ptschemereader(Form("ptscheme/%s", basefile.str().data()), branchlist);
    auto radii = eschemereader.GetDataRef().GetJetRadii();

    Style eschemeStyle{kRed, 24}, ptschemeStyle{kBlue, 25}, ratioStyle{kBlack, 20};
    double ptmax, framemax;
    switch(ultraoption) {
        case 240: ptmax = 320; framemax = 350; break;
        case 300: ptmax = 400; framemax = 450; break;
        default: ptmax = 240; framemax = 300; break;
    };
    Restrictor rangerestrictor(10, ptmax);


    auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), "Comparison reconstruction schemes", 300 * radii.size(), 700);
    plot->Divide(radii.size(), 2);

    int icol(0);
    for(auto radius : radii) {
        std::string rstring(Form("R%02d", int(radius*10.)));
        plot->cd(icol+1);
        std::vector<double> legrange = {0.35, 0.65, 0.94, 0.85}, nolegrange = {};
        SpectrumPad specpad(gPad, radius, "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", {0, framemax, 1e-9, 1}, icol == 0 ? legrange : nolegrange);
        if(!icol) specpad.Label(0.15, 0.85, 0.94, 0.94, Form("10.0 GeV/c < p_{t,jet,det} < %.1f GeV/c", ptmax));

        auto specescheme = rangerestrictor(eschemereader.GetJetSpectrum(radius, branchlist[0])),
             specptscheme = rangerestrictor(ptschemereader.GetJetSpectrum(radius, branchlist[0]));
        specpad.Draw<TH1>(specescheme, eschemeStyle, "E-Scheme");
        specpad.Draw<TH1>(specptscheme, ptschemeStyle, "p_{t}-scheme");

        plot->cd(icol+1+radii.size());
        RatioPad ratiopad(gPad, radius, "p_{t}-scheme / E-scheme", {0, framemax, 0.5, 1.5});
        Ratio *rat = new Ratio(specptscheme, specescheme);
        ratiopad.Draw<Ratio>(rat, ratioStyle);
        icol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}