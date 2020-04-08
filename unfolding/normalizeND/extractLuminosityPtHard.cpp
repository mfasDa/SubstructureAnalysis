#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

void extractLuminosityPtHard(const char *filename = "AnalysisResults.root"){
    const std::map<std::string, double> reflumis = {{"INT7", 0.01}, {"EJ2", 0.44}, {"EJ1", 4.8}};
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    reader->cd("SoftDropResponse_FullJets_R02_INT7");
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto xsechist = static_cast<TH1 *>(histos->FindObject("fHistXsectionAfterSel")),
         luminosity = static_cast<TH1 *>(histos->FindObject("fHistTrialsAfterSel"));
    luminosity->SetDirectory(nullptr);
    luminosity->Divide(xsechist);
    // convert from mb-1 to pb-1
    luminosity->Scale(1e-9);
    luminosity->SetStats(false);
    luminosity->SetTitle();
    luminosity->SetXTitle("p_{t,hard}-bin");
    luminosity->SetYTitle("L_{int} (pb^{-1})");
    
    auto lumiplot = new ROOT6tools::TSavableCanvas("luminosityMC", "Luminosity in pt-hard bins", 800, 600);
    lumiplot->cd();
    lumiplot->SetLogy();
    luminosity->Draw("ep");
    lumiplot->SaveCanvas(lumiplot->GetName());

    auto lumitriggers = new ROOT6tools::TSavableCanvas("lumitriggers", "Luminositu for different triggers", 1200, 600);
    lumitriggers->Divide(3,1);

    int icol = 1; 
    for(auto [trg, lumi] : reflumis) {
        lumitriggers->cd(icol++);
        gPad->SetLogy();
        gPad->SetLeftMargin();
        gPad->SetRightMargin();
        auto histtrigger = static_cast<TH1 *>(luminosity->Clone(Form("luminosity%s", trg.data())));
        histtrigger->SetDirectory(nullptr);
        histtrigger->Scale(1./lumi);
        histtrigger->SetTitle(trg.data());
        histtrigger->SetYTitle(Form("Luminosity / Luminosity(%s)",trg.data()));
        histtrigger->Draw("ep");
    }
    lumitriggers->cd();
    lumitriggers->Update();
    lumitriggers->SaveCanvas(lumitriggers->GetName());
}