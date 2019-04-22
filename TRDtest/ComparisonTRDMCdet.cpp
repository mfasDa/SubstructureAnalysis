#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../struct/GraphicsPad.cxx"
#include "../struct/JetSpectrumReader.cxx"
#include "../struct/Ratio.cxx"
#include "../struct/Rebinner.cxx"
#include "../struct/Restrictor.cxx"
#include "../unfolding/binnings/binningPt1D.C"

void ComparisonTRDMCdet(int ultraoption, const std::string_view sysvar){
    std::stringstream basefile, plotname;
    basefile << "correctedSVD";
    plotname << "comparisonRecoSchemeMCdet";
    if(ultraoption > -1){
        basefile << "_ultra" << ultraoption;
        plotname << "_ultra" << ultraoption;
    } 
    else{
        basefile << "_fine_lowpt";
        plotname << "_fine_lowpt";
    }
    if(sysvar.length()){
        basefile << "_" << sysvar;
        plotname << "_" << sysvar;
    }
    basefile << ".root";
    std::vector<std::string> branchlist = {"mcdet"};
    JetSpectrumReader wTRDreader(Form("withTRD/%s", basefile.str().data()), branchlist), woTRDreader(Form("withoutTRD/%s", basefile.str().data()), branchlist);
    auto radii = wTRDreader.GetDataRef().GetJetRadii();

    Style wTRDStyle{kRed, 24}, woTRDStyle{kBlue, 25}, ratioStyle{kBlack, 20};
    double ptmax, framemax;
    switch(ultraoption) {
        case 240: ptmax = 240; framemax = 300; break;
        case 300: ptmax = 300; framemax = 350; break;
        default: ptmax = 200; framemax = 250; break;
    };
    Restrictor rangerestrictor(10, ptmax);
    Rebinner rebinning(getJetPtBinningNonLinSmearUltra300());


    auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), "Comparison w/wo TRD", 300 * radii.size(), 700);
    plot->Divide(radii.size(), 2);

    int icol(0);
    for(auto radius : radii) {
        std::string rstring(Form("R%02d", int(radius*10.)));
        plot->cd(icol+1);
        std::vector<double> legrange = {0.35, 0.65, 0.94, 0.85}, nolegrange = {};
        SpectrumPad specpad(gPad, radius, "1/N_{ev} d#N/(dp_{t}d#eta) (1/(GeV/c))", {0, framemax, 1e-9, 1}, icol == 0 ? legrange : nolegrange);
        if(!icol) specpad.Label(0.15, 0.85, 0.94, 0.94, Form("10.0 GeV/c < p_{t,jet,det} < %.1f GeV/c", ptmax));

        auto specwith = rangerestrictor(rebinning(wTRDreader.GetJetSpectrum(radius, branchlist[0]))),
             specwithout = rangerestrictor(rebinning(woTRDreader.GetJetSpectrum(radius, branchlist[0])));
        specwith->Scale(1./(40e6));  // undo articial scaling with the number of events
        specwithout->Scale(1./(40e6));
        specpad.Draw<TH1>(specwith, wTRDStyle, "withTRD");
        specpad.Draw<TH1>(specwithout, woTRDStyle, "without TRD");

        plot->cd(icol+1+radii.size());
        RatioPad ratiopad(gPad, radius, "without TRD / with TRD", {0, framemax, 0.5, 1.5});
        Ratio *rat = new Ratio(specwithout, specwith);
        ratiopad.Draw<Ratio>(rat, ratioStyle);
        icol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}