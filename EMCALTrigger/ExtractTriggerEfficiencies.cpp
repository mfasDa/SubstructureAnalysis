#ifndef __CLING__
#include <array>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TROOT.h>
#include <TTreeReader.h>
#endif

TTree *GetTree(TFile &reader){
    TTree *result(nullptr);
    for(auto k : TRangeDynCast<TKey>(reader.GetListOfKeys())){
        if(std::string(k->GetName()).find("jetSubstructureMerged") != std::string::npos){
            result = k->ReadObject<TTree>();
            break;
        }
    }
    return result;
}

TH1 *GetDetLevelJetSpectrum(std::string_view filename) {
    TH1 *pthist = new TH1D("SpecJetPtDet", "", 200, 0., 200.);
    pthist->SetDirectory(nullptr);
    auto datafile = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
    gROOT->cd();
    TTreeReader treereader(GetTree(*datafile));
    TTreeReaderValue<double> ptrec(treereader, "PtJetRec");
    TTreeReaderValue<double> nefrec(treereader, "NEFRec");
    TTreeReaderValue<double> pythiaweight(treereader, "PythiaWeight");
    for(auto en : treereader){
        if(*nefrec > 0.97) continue;            // Make cut on neutral energy fraction removing single cluster jets
        pthist->Fill(*ptrec, *pythiaweight);
    }
    return pthist;
}

void ExtractTriggerEfficiencies(double radius){
    std::array<std::string, 3> triggers = {{"INT7", "EJ1", "EJ2"}};
    TH1 *mbref = nullptr;
    std::vector<TH1 *> emcaltriggers;
    for(auto t : triggers) {
        std::stringstream filename, histname;
        filename << "JetSubstructureTree_R" << std::setw(2) << std::setfill('0') << int(radius * 10) << "_" << t << "_merged.root";
        std::cout << "Analysing trigger " << t << " with filename " << filename.str() <<  std::endl;
        auto hist = GetDetLevelJetSpectrum(filename.str());
        histname << "specPt" << t;
        hist->SetName(histname.str().data());
        hist->GetXaxis()->SetTitle("p_{t,jet} (GeV/c)");
        hist->GetYaxis()->SetTitle("d#sigma/dp_{t,jet} (mb/(GeV/c))");
        if(t.find("INT7") != std::string::npos) mbref = hist;
        else emcaltriggers.emplace_back(hist);
    }

    std::vector<TH1 *> efficiencies;
    for(auto e : emcaltriggers){
        std::string triggername = e->GetName();
        triggername.replace(triggername.find("specPt"), 6, "");
        auto newhist = new TH1D(*((TH1D *)e));
        std::stringstream histname;
        histname << "efficiency" << triggername;
        newhist->SetDirectory(nullptr);
        newhist->SetName(histname.str().data());
        newhist->Divide(e, mbref, 1., 1., "b");
        newhist->GetYaxis()->SetTitle("Trigger efficiency");
        efficiencies.emplace_back(newhist);
    }

    auto writer = std::unique_ptr<TFile>(TFile::Open(Form("triggereffficiencies_R%02d.root", int(radius*10.)), "RECREATE"));
    writer->mkdir("spectra");
    writer->cd("spectra");
    mbref->Write();
    for(auto s : emcaltriggers) s->Write();
    writer->mkdir("efficiencies");
    writer->cd("efficiencies");
    for(auto e : efficiencies) e->Write();
}