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
        if(treename.find("jetSubstructure") != std::string::npos) { 
            result = treename;
            break;
        }
    }
    return result;
}

void makeJetPtCorrelation(std::string_view filemc){
    auto filereader = std::unique_ptr<TFile>(TFile::Open(filemc.data(), "READ"));
    TTreeReader treereader(Get<TTree>(*filereader, FindTree(*filereader).data()));
    TTreeReaderValue<double>    ptrec(treereader, "PtJetRec"),
                                ptsim(treereader, "PtJetSim"),
                                nefrec(treereader, "NEFRec"),
//                                weight(treereader, "PythiaWeight");
                                weight(treereader, "EventWeight");
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
        histo->Fill(*ptsim, *ptrec, *weight);
        histodiff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), *weight);
        profilediff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), *weight);
        hpttrue->Fill(*ptsim, *weight);
        hptrec->Fill(*ptrec, *weight);
        rawcounts->Fill(*ptsim);
    }

    auto histwriter = std::unique_ptr<TFile>(TFile::Open("EnergyScale.root", "RECREATE"));
    histwriter->cd();
    histo->Write();
    histodiff->Write();
    profilediff->Write();
    hpttrue->Write();
    hptrec->Write();
    rawcounts->Write();
}