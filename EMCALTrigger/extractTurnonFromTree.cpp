#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../helpers/root.C"
#include "../helpers/substructuretree.C"

const std::array<std::string, 3> triggers = {"INT7", "EJ1", "EJ2"};

TH1 *extractRawSpectrum(const std::string_view filename){
    const double binning[] = {10., 15., 20., 25., 30., 35., 40., 50., 60., 70, 80, 90., 100., 120., 140, 160., 200.};
    ROOT::RDataFrame dataframe(GetNameJetSubstructureTree(filename), filename);
    auto rawspectrum = dataframe.Histo1D({"rawspectrum", "; p_{t} (GeV/c), N_{jet}", sizeof(binning)/sizeof(double)-1, binning}, "PtJetRec");
    auto result = new TH1D(*rawspectrum);
    result->SetDirectory(nullptr);
    return result;
}

std::map<std::string, int> readNriggers(const std::string_view filename = "merged_1617/AnalysisResults_split.root"){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::map<std::string, int> nevents;
    for(auto trg : triggers) {
        std::stringstream dirname;
        dirname << "JetSubstructure_FullJets_R02_" << trg;
        reader->cd(dirname.str().data());
        auto list = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto evhist = static_cast<TH1 *>(list->FindObject("hEventCounter"));
        nevents[trg] = evhist->GetBinContent(1);
    }    
    return nevents;
}

void extractTurnonFromTree(){
    std::map<int, TH1 *> ej1turnon, ej2turnon;
    std::map<int, TH1 *> ej1raw, ej2raw, int7raw;
    auto norm = readNriggers();
    for(auto radius : ROOT::TSeqI(2, 6.)){
        std::map<std::string, TH1 *> spectra;
        for(auto trg : triggers){
            std::stringstream filename;
            if(trg == "INT7") filename << "merged_1617/";
            else filename << "merged_17/";
            filename << "JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << radius << "_" << trg << ".root";
            spectra[trg] = extractRawSpectrum(filename.str());
            spectra[trg]->Scale(1./norm[trg]);
            spectra[trg]->SetName(Form("%sspectrum_R%02d", trg.data(), radius));
            if(trg == "INT7") int7raw[radius] = spectra[trg];
            else if(trg == "EJ1") ej1raw[radius] = spectra[trg];
            else if(trg == "EJ2") ej2raw[radius] = spectra[trg];
        }
        auto myej1 = histcopy(spectra["EJ1"]);
        myej1->SetName(Form("EJ1turnon_R%02d", radius));
        myej1->Divide(spectra["INT7"]);
        ej1turnon[radius] = myej1;
        auto myej2 = histcopy(spectra["EJ2"]);
        myej2->SetName(Form("EJ2turnon_R%02d", radius));
        myej2->Divide(spectra["INT7"]);
        ej2turnon[radius] = myej2;
    }

    std::map<int, Style> rstyles = {{2, {kRed, 24}}, {3, {kGreen, 25}}, {4., {kBlue, 26}}, {5, {kViolet, 26}}};

    auto plot = new ROOT6tools::TSavableCanvas("triggerturnon", "Trigger turnon curves", 1200, 800);
    plot->Divide(2, 1);
    plot->cd(1);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.04);
    (new ROOT6tools::TAxisFrame("turnonframeEJ1", "p_{t,jet} (GeV/c)", "Trigger / Min. Bias", 0., 200., 0., 10000.))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.72, 0.15, 0.89, 0.4);
    leg->Draw();
    for(auto r : ej1turnon) {
        rstyles[r.first].SetStyle<TH1>(*r.second);
        r.second->Draw("epsame");
        leg->AddEntry(r.second, Form("R=%.1f", double(r.first)/10.), "lep");
    }
    plot->cd(2);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.04);
    (new ROOT6tools::TAxisFrame("turnonframeEJ2", "p_{t,jet} (GeV/c)", "Trigger / Min. Bias", 0., 200., 0., 4000.))->Draw("axis");
    for(auto r : ej2turnon) {
        rstyles[r.first].SetStyle<TH1>(*r.second);
        r.second->Draw("epsame");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());

    std::unique_ptr<TFile> writer(TFile::Open("triggerturnon.root", "RECREATE"));
    writer->mkdir("ej1turnon");
    writer->cd("ej1turnon");
    for(auto r : ej1turnon) r.second->Write();
    writer->mkdir("ej2turnon");
    writer->cd("ej2turnon");
    for(auto r : ej2turnon) r.second->Write();

    writer->mkdir("INT7");
    writer->cd("INT7");
    for(auto r : int7raw) r.second->Write();
    writer->mkdir("EJ1");
    writer->cd("EJ1");
    for(auto r : ej1raw) r.second->Write();
    writer->mkdir("EJ2");
    writer->cd("EJ2");
    for(auto r : ej2raw) r.second->Write();
}