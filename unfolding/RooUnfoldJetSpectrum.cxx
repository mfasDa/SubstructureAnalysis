#ifndef __CLING__
#include <RStringView.h>
#include <ROOT/TDataFrame.hxx>
#include <TH1.h>
#include <RooUnfoldResponse.h>
#include <RooUnfoldBayes.h>
#include <RooUnfoldSVD.h>
#endif

void RooUnfoldJetSpectrum(std::string_view filedata, std::string_view filemc){
    double ptbinning[] = {0., 5., 10., 15., 20., 25, 30., 35., 40., 45., 50., 60, 70., 80., 100., 120., 140., 160., 180., 200., 240., 280., 320., 360., 400.};
    TH1D hTrue("hTrueJetPt", "true jet pt; p_{t,j} (GeV/c); N_{jet}", sizeof(ptbinning)/sizeof(double)-1, ptbinning),
         hSmeared("hSmearedJetPt", "smeared jet pt; p_{t,j} (GeV/c); N_{jet}", sizeof(ptbinning)/sizeof(double)-1, ptbinning),
         hMeasured("hMeasuredJetPt", "measured jet pt; p_{t,j} (GeV/c); N_{jet}", sizeof(ptbinning)/sizeof(double)-1, ptbinning);
    RooUnfoldResponse res;
    res.Setup(&hSmeared, &hMeasured);

    ROOT::Experimental::TDataFrame datajetframe("jetSubstructureTree", filedata.data()), mcjetframe("jetSubstructureTreeMerged", filemc.data());
    auto specdata = datajetframe.Filter("NEFRec < 1.").Histo1D(hMeasured, "PtJetRec");
    auto specmctrue = mcjetframe.Histo1D(hTrue, "PtJetSim", "PythiaWeight");
    auto specmcsmear = mcjetframe->Filter("NEFRec < 1.")
    mcjetframe.Filter("NEFRec < 1.").Foreach([&res](double ptrec, double ptsim){ res.Fill(ptrec, ptsim); }, {"PtJetRec", "PtJetSim"});

    // Do unfolding
    for(auto it : ROOT::TSeqI(1, 11)) {
        RooUnfoldBayes unfolderBayes
    }
}