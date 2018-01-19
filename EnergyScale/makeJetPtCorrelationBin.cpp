
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

void makeJetPtCorrelationBin(std::string_view filemc, int bin){
    std::vector<double> weights = {
        3.830920e-06,
        3.556541e-06,
        3.989838e-07,
        3.579387e-08,
        3.072742e-09,
        4.021247e-10,
        3.986867e-11,
        6.431317e-12,
        1.603337e-12,
        2.696621e-13,
        1.277079e-13
    };
    auto filereader = make_unique<TFile>(TFile::Open(filemc.data(), "READ"));
    TTreeReader treereader(Get<TTree>(filereader.get(), "jetSubstructure"));
    TTreeReaderValue<double>    ptrec(treereader, "PtJetRec"),
                                ptsim(treereader, "PtJetSim");
    TH1 *hpttrue = new TH1D("hPtTrue", "true pt", 200, 0, 200);                            
    auto hptrec = new TH1D("hPtRec", "true pt", 200, 0, 200);                            
    TH2 *histo = new TH2D("ptcorr", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 40, 0., 200., 15, 0., 300.);
    TH2 *histodiff = new TH2D("ptdiff", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 40, 0., 200., 26, -1.05, 1.05);
    TProfile *profilediff = new TProfile("ptdiffmean", "; p_{t, part} (GeV/c); p_{t, det} (GeV/c)", 40, 0., 200.);
    TH1 *rawcounts = new TH1D("rawcounts", "true pt distribution, unweighted", 40, 0., 200.);
    auto weight = weights[bin];
    for(auto en : treereader){
        //if(*ptrec <= DBL_EPSILON) continue; // Filter matched jets
        if(*ptrec < 10) continue;   // in order to match with Leticia's trees
        histo->Fill(*ptsim, *ptrec, weight);
        histodiff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), weight);
        profilediff->Fill(*ptsim, (*ptrec - *ptsim)/(*ptsim), weight);
        hpttrue->Fill(*ptsim, weight);
        hptrec->Fill(*ptrec, weight);
        rawcounts->Fill(*ptsim);
    }

    auto histwriter = make_unique<TFile>(TFile::Open("EnergyScale.root", "RECREATE"));
    histwriter->cd();
    histo->Write();
    histodiff->Write();
    profilediff->Write();
    hpttrue->Write();
    hptrec->Write();
    rawcounts->Write();
}