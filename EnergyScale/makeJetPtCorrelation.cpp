#ifndef __CLING__
#include <cfloat>
#include <memory>
#include <RStringView.h>
#include <TFile.h>
#include <TH2.h>
#include <TProfile.h>
#include <TTreeReader.h>
#include <TTree.h>
#endif

template<typename t>
t *Get(TFile &reader, const char *key){
    //t *result(nullptr);
    auto result = static_cast<t *>(reader.Get(key));
    return result;
}

std::string FindTree(TFile &reader) {
    auto result = std::string();
    for(auto k : TRangeDynCast<TKey>(reader.GetListOfKeys())){
        auto treename = std::string(k->GetName());
        if(treename.find("jetSubstructure") != std::string::npos || treename.find("JetSubstructure") != std::string::npos) { 
            result = treename;
            break;
        }
    }
    return result;
}

void makeJetPtCorrelation(std::string_view filemc, bool withWeight = true){
    bool debug = false;
    auto filereader = std::unique_ptr<TFile>(TFile::Open(filemc.data(), "READ"));
    TTreeReader treereader(Get<TTree>(*filereader, FindTree(*filereader).data()));
    TTreeReaderValue<double>    ptrec(treereader, "PtJetRec"),
                                ptsim(treereader, "PtJetSim"),
                                nefrec(treereader, "NEFRec");
    std::unique_ptr<TTreeReaderValue<double>> weight;
    if(withWeight){
        weight = std::unique_ptr<TTreeReaderValue<double>>(new TTreeReaderValue<double>(treereader, "PythiaWeight"));
        //weight = std::unique_ptr<TTreeReaderValue<double>>(new TTreeReaderValue<double>(treereader, "EventWeight"));

    }
    TH1 *hpttrue = new TH1D("hPtTrue", "true pt", 200, 0, 200);                            
    auto hptrec = new TH1D("hPtRec", "true pt", 200, 0, 200);                            
    TH2 *histo = new TH2D("ptcorr", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 40, 0., 200., 15, 0., 300.);
    TH2 *histodiff = new TH2D("ptdiff", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 40, 0., 200., 26, -1.05, 1.05);
    TProfile *profilediff = new TProfile("ptdiffmean", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 40, 0., 200.);
    TH1 *rawcounts = new TH1D("", "true pt distribution, unweighted", 40, 0., 200.);
    for(auto en : treereader){
        //if(*ptrec <= DBL_EPSILON) continue; // Filter matched jets
        if(*nefrec > 0.97) continue;
        if(*ptrec < 10) continue;   // in order to match with Leticia's trees
        double usedweight = 1.;
        if(weight.get() != nullptr) {
            usedweight = *(*weight);
        } else {
            if(debug)std::cout << "No weight found" << std::endl;
        }
        if(debug) std::cout << "Using weight " << usedweight << std::endl;
        histo->Fill(*ptsim, *ptrec, usedweight);
        histodiff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), usedweight);
        profilediff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), usedweight);
        hpttrue->Fill(*ptsim, usedweight);
        hptrec->Fill(*ptrec, usedweight);
        rawcounts->Fill(*ptsim);
    }

    std::string tag = static_cast<std::string>(filemc);
    tag.replace(tag.find("JetSubstructureTree_"), strlen("JetSubstructureTree_"), "");
    tag.replace(tag.find(".root"), 5, "");

    auto histwriter = std::unique_ptr<TFile>(TFile::Open(Form("EnergyScale_%s.root", tag.data()), "RECREATE"));
    histwriter->cd();
    histo->Write();
    histodiff->Write();
    profilediff->Write();
    hpttrue->Write();
    hptrec->Write();
    rawcounts->Write();
}