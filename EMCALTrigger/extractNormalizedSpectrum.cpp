#ifndef __CLING__
#include <array>
#include <memory>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TTreeReader.h>
#endif

std::vector<double> getJetPtBinning(){
    std::vector<double> binning;
    auto max = 0.;
    binning.emplace_back(max);
    while(max < 10.){
        max += 1.;
        binning.emplace_back(max);
    }
    while(max < 20.){
        max += 2.;
        binning.emplace_back(max);
    }
    while(max < 50.){
        max += 5.;
        binning.emplace_back(max);
    }
    while(max < 100.){
        max += 10.;
        binning.emplace_back(max);
    }
    while(max < 200.){
        max += 20.;
        binning.emplace_back(max);
    }
    return binning;
}

double ExtractLumi(const std::string_view container){
    auto reader = std::unique_ptr<TFile>(TFile::Open("AnalysisResults.root", "READ"));
    reader->cd(container.data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto lumihist = static_cast<TH1 *>(histlist->FindObject("hLumiMonitor"));
    auto lumi = lumihist->GetBinContent(lumihist->GetXaxis()->FindBin("CENT"));
    return lumi;
}

double ExtractEvents(const std::string_view container){
    auto reader = std::unique_ptr<TFile>(TFile::Open("AnalysisResults.root", "READ"));
    reader->cd(container.data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto lumihist = static_cast<TH1 *>(histlist->FindObject("hEventCounter"));
    auto lumi = lumihist->GetBinContent(1);
    return lumi;
}

TH1 * ExtractRawYield(const std::string_view filename, bool weighted){
    auto binning = getJetPtBinning();
    auto hpt = new TH1D("hpt", "", binning.size()-1, binning.data());
    hpt->SetDirectory(nullptr);
    hpt->Sumw2();
    auto filereader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
    TTreeReader treereader(static_cast<TKey *>(filereader->GetListOfKeys()->At(0))->ReadObject<TTree>());
    TTreeReaderValue<double> ptrec(treereader, "PtJetRec"),
                             eventweight(treereader, "EventWeight");
    for(auto en : treereader) {
        hpt->Fill(*ptrec, weighted ? *eventweight : 1);
    }
    // Correct for bin width
    for(auto b : ROOT::TSeqI(1, hpt->GetXaxis()->GetNbins()+1)){
        auto bw = hpt->GetXaxis()->GetBinWidth(b);
        hpt->SetBinContent(b, hpt->GetBinContent(b)/bw);
        hpt->SetBinError(b, hpt->GetBinError(b)/bw);
    }
    return hpt;
}

TH1 *extractNormalizedSpectrum(double r, const std::string_view trigger) {
    std::cout << "Extracting spectrum for trigger " << trigger << " and radius " << r << std::endl;
    std::stringstream contname, treefile, histname, jettag;
    jettag << "R" << std::setw(2) << std::setfill('0') << int(r * 10.) << "_" << trigger;
    contname << "JetSubstructure_" << jettag.str();
    treefile << "JetSubstructureTree_" << jettag.str() << ".root";
    histname << "hPtJet" << jettag.str();

    auto isINT7 = (trigger.find("INT7") != std::string::npos);
    auto hist = ExtractRawYield(treefile.str(), !isINT7);
    hist->SetName(histname.str().data());
    hist->Scale(1./(isINT7 ? ExtractEvents(contname.str()) : ExtractLumi(contname.str())));
    return hist;
}

void extractNormalizedSpectrum(bool emcmode){
    auto writer = std::unique_ptr<TFile>(TFile::Open("normalized.root", "RECREATE"));
    std::array<double, 2> radii = {{0.2, 0.4}};
    std::array<std::string, 3> triggers = {"EJ1", "EJ2", "INT7"};
    for(auto t: triggers) {
        if((emcmode && (t.find("INT7") != std::string::npos)) || (!emcmode && (t.find("EJ") != std::string::npos))) continue;
        for(auto r : radii) {
            auto h = extractNormalizedSpectrum(r, t);
            writer->cd();
            h->Write();
        }
    }
}