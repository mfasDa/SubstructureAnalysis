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
t *Get(TFile *reader, const char *key){
    t *result(nullptr);
    result = static_cast<t *>(reader->Get(key));
    return result;
}

template<typename t>
std::unique_ptr<t> make_unique(t *object){
    std::unique_ptr<t> ptr(object);
    return ptr;
}

void makeJetPtCorrelationsLeticia(std::string_view filemc = "DetRespPythia7TeVMarkus.root"){
    auto filereader = make_unique<TFile>(TFile::Open(filemc.data(), "READ"));
    TTreeReader treereader(Get<TTree>(filereader.get(), "newtree"));
    TTreeReaderValue<float>    ptrec(treereader, "ptJet"),
                               ptsim(treereader, "ptJetMatch");
    TTreeReaderValue<double>   weight(treereader, "weightPythiaFromPtHard");
    TH1 *hpttrue = new TH1D("hPtTrue", "true pt", 200, 0, 200);                            
    auto hptrec = new TH1D("hPtRec", "true pt", 200, 0, 200);                            
    TH2 *histo = new TH2D("ptcorr", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 60, 0., 300., 15, 0., 300.);
    TH2 *histodiff = new TH2D("ptdiff", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 60, 0., 300., 26, -1.05, 1.05);
    TProfile *profilediff = new TProfile("ptdiffmean", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 60, 0., 300.);
    for(auto en : treereader){
        //if(*ptrec <= DBL_EPSILON) continue; // Filter matched jets
        if(*ptrec < 10) continue;   // in order to match with Leticia's trees
        histo->Fill(*ptsim, *ptrec, *weight);
        histodiff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), *weight);
        profilediff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), *weight);
        hpttrue->Fill(*ptsim, *weight);
        hptrec->Fill(*ptrec, *weight);
    }

    auto histwriter = make_unique<TFile>(TFile::Open("EnergyScale.root", "RECREATE"));
    histwriter->cd();
    histo->Write();
    histodiff->Write();
    profilediff->Write();
    hpttrue->Write();
    hptrec->Write();
}