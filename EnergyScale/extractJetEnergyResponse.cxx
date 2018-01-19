#ifndef __CLING__
#include <memory>
#include <RStringView.h>
#include <ROOT/TDataFrame.hxx>
#include <TH2.h>
#include <TFile.h>
#endif

void extractJetEnergyResponse(std::string_view inputfile){
    double ptbinning[] = {0., 5., 10., 15., 20., 25, 30., 35., 40., 45., 50., 60, 70., 80., 100., 120., 140., 160., 180., 200., 240., 280., 320., 360., 400.};
    ROOT::Experimental::TDataFrame substructureframe("jetSubstructureMerged", inputfile);
    TH2D histtemplatePT("ptresponse", "p_{t}-response ;p_{t,rec} (GeV/c); p{t_sim} (*GeV/c)", sizeof(ptbinning)/sizeof(double)-1, ptbinning, sizeof(ptbinning)/sizeof(double)-1, ptbinning);
    TH2D histtemplateEnergy("energyresponse", "energy response; E{rec} (GeV); E_{sim} (GeV)", sizeof(ptbinning)/sizeof(double)-1, ptbinning, sizeof(ptbinning)/sizeof(double)-1, ptbinning);
    auto ptrepsonse = substructureframe.Filter("NEFRec<1 && NEFSim<1").Histo2D(histtemplatePT, "PtJetRec", "PtJetSim", "PythiaWeight");
    auto energyresponse = substructureframe.Filter("NEFRec<1 && NEFSim<1").Histo2D(histtemplateEnergy, "PtJetRec", "PtJetSim", "PythiaWeight");

    std::unique_ptr<TFile> writer(TFile::Open("JetResponse.root", "RECREATE"));
    writer->cd();
    ptrepsonse->Write();
    energyresponse->Write();
}