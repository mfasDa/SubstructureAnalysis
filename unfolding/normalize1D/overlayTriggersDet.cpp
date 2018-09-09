#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/substructuretree.C"
#include "../binnings/binningPt1D.C"

std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};

TH1 *readSmeared(const std::string_view inputfile, bool weighted, bool downscaleweighted){
    auto binning = getJetPtBinningNonLinSmearLarge();
    ROOT::RDataFrame df(GetNameJetSubstructureTree(inputfile), inputfile);
    TH1 *result(nullptr);
    if(weighted){
        auto hist = df.Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec", "PythiaWeight");
        result = histcopy(hist.GetPtr());
    } else if(downscaleweighted){
        auto hist = df.Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec", "EventWeight");
        result = histcopy(hist.GetPtr());
    } else {
        auto hist = df.Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec");
        result = histcopy(hist.GetPtr());
    }
    result->SetDirectory(nullptr);
    return result;
}

std::vector<TH1 *> extractCENTNOTRDCorrection(std::string_view filename){
    auto binning = getJetPtBinningNonLinSmearLarge();
    ROOT::RDataFrame df(GetNameJetSubstructureTree(filename), filename);
    TH1 *result(nullptr);
    auto selCENT = df.Filter("TriggerClusterIndex < 1");
    auto speccentnotrd = df.Histo1D({"speccentnotrd", "Spectrum centnotrd", static_cast<int>(binning.size()) - 1, binning.data()}, "PtJetRec"),
         speccent = selCENT.Histo1D({"speccent", "Spectrum cent", static_cast<int>(binning.size()) - 1, binning.data()}, "PtJetRec");
    speccent->Sumw2();
    speccentnotrd->Sumw2();
    auto correction = histcopy(speccentnotrd.GetPtr());
    correction->SetNameTitle("CENTNOTRDCorrection", "Correction for the unmeasured CENTNOTRD Luminosity");
    correction->SetDirectory(nullptr);
    correction->Divide(speccentnotrd.GetPtr(), speccent.GetPtr(), 1., 1., "b");
    auto rescentnotrd = histcopy(speccentnotrd.GetPtr()), rescent = histcopy(speccent.GetPtr());
    rescentnotrd->SetDirectory(nullptr);
    rescent->SetDirectory(nullptr);
    return {rescentnotrd, rescent, correction};
}

double extractLumiCENT(const std::string_view filename){
    std::pair<double, double> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("JetSubstructure_FullJets_R02_INT7");
    auto list = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto lumihist = static_cast<TH1 *>(list->FindObject("hLumiMonitor"));
    auto lumicent = lumihist->GetBinContent(lumihist->GetXaxis()->FindBin("CENT"));
    return lumicent;
}

std::map<std::string, int> readNriggers(const std::string_view filename = "data/merged_1617/AnalysisResults_split.root"){
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

void overlayTriggersDet(double r){
    auto lumiCENT = extractLumiCENT("data/merged_17/AnalysisResults_split.root");
    auto centnotrdCorrection = extractCENTNOTRDCorrection(Form("data/merged_17/JetSubstructureTree_FullJets_R%02d_EJ1.root", int(r*10.)));
    TF1 fit("centnotrdcorrfit", "pol0", 0., 200.);
    centnotrdCorrection[2]->Fit(&fit, "N", "", 20., 200.);
    std::cout << "Using CENTNOTRD correction factor " << fit.GetParameter(0) << std::endl;
    auto lumiCENTNOTRD = lumiCENT * fit.GetParameter(0);
    auto lumihist = new TH1D("luminosities", "Luminosities", 3, 0., 3.);
    lumihist->SetDirectory(nullptr);
    lumihist->GetXaxis()->SetBinLabel(1, "INT7");
    lumihist->GetXaxis()->SetBinLabel(2, "CENT");
    lumihist->GetXaxis()->SetBinLabel(3, "CENTNOTRD");
    lumihist->SetBinContent(2, lumiCENT);
    lumihist->SetBinContent(3, lumiCENTNOTRD);
    std::map<std::string, TH1 *> mcspectra, dataspectra;
    // Read MC specta
    for(const auto &trg : triggers) {
        std::stringstream filename;
        filename << "mc/merged_calo/JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_" << trg << "_merged.root";
        auto spec = readSmeared(filename.str(), true, false);
        spec->SetName(Form("mcspec_R%02d_%s", int(r*10.), trg.data()));
        mcspectra[trg] = spec;
    }
    // Read data specta
    for(const auto &trg : triggers) {
        std::stringstream filename;
        filename << "data/" << (trg == "INT7" ? "merged_1617" : "merged_17") << "/JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_" << trg << ".root";
        auto spec = readSmeared(filename.str(), false, trg == "EJ2");
        spec->SetName(Form("dataspec_R%02d_%s", int(r*10.), trg.data()));
        dataspectra[trg] = spec;
    }
    // get weights and renormalize data spectra
    auto weights = readNriggers();
    std::map<std::string, TH1 *> hnorm;
    for(auto &spec : dataspectra) {
        auto trgweight = weights.find(spec.first)->second;
        if(spec.first == "EJ1") {
            spec.second->Scale(1./lumiCENTNOTRD);
        } else if(spec.first == "EJ2") {
            spec.second->Scale(1./lumiCENT);
        } else {
            lumihist->SetBinContent(1, trgweight);
            spec.second->Scale(1./trgweight);
        }
        auto normhist = new TH1D(Form("norm%s", spec.first.data()), Form("event count trigger %s", spec.first.data()), 1, 0.5, 1.5);
        normhist->SetBinContent(1, trgweight);
        hnorm[spec.first] = normhist;
    }
    // build efficiencies, correct triggered spectra
    std::map<std::string, TH1 *> efficiencies;
    auto reference = mcspectra.find("INT7")->second;
    for(auto &trg : triggers) {
        if(trg == "INT7") continue;
        auto eff = histcopy(mcspectra.find(trg)->second);
        eff->SetName(Form("Efficiency_R%02d_%s", int(r*10.), trg.data()));
        eff->Divide(eff, reference, 1., 1., "b");
        efficiencies[trg] = eff;
        auto tocorrect = dataspectra.find(trg)->second;
        tocorrect->Divide(eff);
    }

    // ratio trigger / min bias
    std::map<std::string, TH1 *> ratios;
    auto dataref = dataspectra.find("INT7")->second;
    for(auto &trg : triggers){
        if(trg == "INT7") continue;
        auto ratio = histcopy(dataspectra.find(trg)->second);
        ratio->SetName(Form("%soverMB_R%02d", trg.data(), int(r*10)));
        ratio->Divide(dataref);
        ratios[trg] = ratio;
    }

    // plot ratios
    auto plot = new ROOT6tools::TSavableCanvas(Form("corrdet_R%02d", int(r*10.)), Form("Comparison Correction R=%.1f", r), 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("compframe", "p_{t,det} (GeV/c)", "Trigger / Min. Bias", 0., 200., 0., 2.))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("jets, R=%.1f", r)))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.8, 0.89, 0.89);
    leg->SetNColumns(2);
    leg->Draw();
    auto ej1 = ratios.find("EJ1")->second;
    Style{kRed,24}.SetStyle<TH1>(*ej1);
    ej1->Draw("epsame");
    auto ej2 = ratios.find("EJ2")->second;
    Style{kBlue,25}.SetStyle<TH1>(*ej2);
    ej2->Draw("epsame");
    leg->AddEntry(ej1, "EJ1", "lep");
    leg->AddEntry(ej2, "EJ2", "lep");
    plot->Update();
    plot->SaveCanvas(plot->GetName());

    // write everything
    std::unique_ptr<TFile> writer(TFile::Open(Form("correctdet_R%02d.root", int(r*10.)), "RECREATE"));
    writer->cd();
    lumihist->Write();
    for(auto m : mcspectra) {normalizeBinWidth(m.second); m.second->Write();}
    for(auto d : dataspectra) {normalizeBinWidth(d.second); d.second->Write();}
    for(auto e : efficiencies) e.second->Write();
    for(auto n : hnorm) n.second->Write();
    for(auto r : ratios) r.second->Write();
    for(auto c : centnotrdCorrection) c->Write();
}