#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../struct/GraphicsPad.cxx"
#include "../struct/Ratio.cxx"

TH1 *readSpectrum(const std::string_view filename, const std::string_view trigger) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::string listname = "AliEmcalTrackingQATask_histos";
    if(trigger.find("E") != std::string::npos) listname = Form("AliEmcalTrackingQATask_%s_histos", trigger.data());
    auto histos = static_cast<TList *>(reader->Get(listname.data()));
    auto norm = static_cast<TH1 *>(histos->FindObject("fHistEventCount"))->GetBinContent(1);
    auto tracksparse = static_cast<THnSparse *>(histos->FindObject("fTracks"));
    // Restrict to EMCAL
    tracksparse->GetAxis(1)->SetRangeUser(-0.6, 0.6);
    tracksparse->GetAxis(2)->SetRangeUser(1.4, 3.1);
    auto spec1D = tracksparse->Projection(0);
    spec1D->SetDirectory(nullptr);
    spec1D->Scale(1./norm);
    spec1D->Scale(1., "width");
    return spec1D;
}

void CompareTracksTRD(){
    auto plot = new ROOT6tools::TSavableCanvas("ComparisonTracksTRD", "Comparison track spectra with/without TRD", 1400, 1000);
    plot->Divide(3,2);
    const std::array<std::string, 3> triggers = {{"INT7", "EJ2", "EJ1"}};
    int icol = 0;
    Style withstyle{kRed, 24}, withoutstyle{kBlue, 25}, ratiostyle{kBlack, 20};
    for(const auto &trg : triggers){
        std::stringstream basename;
        if(trg == "INT7") basename << "INT7";
        else basename << "EMC";
        basename << "/AnalysisResults.root";
        auto specwithout = readSpectrum(Form("withoutTRD/data/%s", basename.str().data()), trg),
             specwith = readSpectrum(Form("withTRD/data/%s", basename.str().data()), trg);
        specwith->SetName(Form("specwith_%s", trg.data()));
        specwithout->SetName(Form("specwithout_%s", trg.data()));
        
        plot->cd(icol+1);
        gPad->SetLogy();
        GraphicsPad specpad(gPad);
        specpad.Margins(0.14, 0.04, -1., 0.04);
        specpad.Frame(Form("specpad%s", trg.data()), "p_{t,det} (GeV/c)", "with/without TRD", 0., 200., 1e-9, 100);
        specpad.Label(0.15, 0.15, 0.3, 0.22, trg);
        if(icol == 0) specpad.Legend(0.35, 0.65, 0.94, 0.94);
        specpad.Draw<TH1>(specwith, withstyle, "with TRD");
        specpad.Draw<TH1>(specwithout, withoutstyle, "without TRD");

        plot->cd(icol+4);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.14, 0.04, -1., 0.04);
        ratiopad.Frame(Form("ratiopad%s", trg.data()), "p_{t,det} (GeV/c)", "", 0., 200., 0.5, 1.5);
        Ratio *trackratio = new Ratio(specwith, specwithout);
        ratiopad.Draw<Ratio>(trackratio, ratiostyle, "ratio");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}