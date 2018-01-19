#ifndef __CLING__
#include <ROOT/TDataFrame.hxx>
#include <ROOT/TSeq.hxx>
#include <TArrayD.h>
#include <TFile.h>
#include <RStringView.h>
#include <memory>
#endif

TArrayD makeLinearBinning(int nbins, double min, double max) {
    TArrayD result(nbins+1);
    result[0] = min;
    double stepwidth = (max - min)/static_cast<double>(nbins);
    for(auto r : ROOT::TSeqI(1, nbins+1)) result[r] = min + r * stepwidth;
    return result;
}

void extractYield2D(const std::string_view filename = "AnalysisResults.root", bool useweight = false){
    TString tag(filename);
    tag.ReplaceAll("JetSubstructureTree_", "");
    tag.ReplaceAll(".root", "");
    double jetptbins[] = {20., 40., 60., 80., 100., 120., 140., 160., 180, 200.},
           zgbins[] = {0., 0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
    TArrayD massbinning = makeLinearBinning(80, 0., 40.),
            angularitybinning = makeLinearBinning(20, 0., 0.2), ptdbinning = makeLinearBinning(10, 0., 1.);
    TString treename = "jetSubstructure";
    if(useweight) treename+= "Merged";
    ROOT::Experimental::TDataFrame jetsubstructureFrame(treename, filename.data());
    TH2D zgdistmodel("zgdist", "zgdist; p_{t,jet} (GeV/c); zg", sizeof(jetptbins)/sizeof(double)-1, jetptbins, sizeof(zgbins)/sizeof(double)-1, zgbins),
         mgdistmodel("mgdist", "mgdist; p_{t,jet} (GeV/c); m_{g} (GeV/c^{2}",sizeof(jetptbins)/sizeof(double)-1, jetptbins, massbinning.GetSize()-1, massbinning.GetArray()),
         mdistmodel("mdist", "mdist; p_{t,jet} (GeV/c); m_{g} (GeV/c^{2}",sizeof(jetptbins)/sizeof(double)-1, jetptbins, massbinning.GetSize()-1, massbinning.GetArray()),
         angularitymodel("angdist", "angdist; p_{t,jet} (GeV/c); g", sizeof(jetptbins)/sizeof(double)-1, jetptbins, angularitybinning.GetSize()-1, angularitybinning.GetArray()),
         ptdmodel("ptd", "ptddist; p_{t,jet} (GeV/c); p_{t, D} (GeV/c)", sizeof(jetptbins)/sizeof(double)-1, jetptbins, ptdbinning.GetSize()-1, ptdbinning.GetArray());
    if(!useweight) {
        auto distZg = jetsubstructureFrame.Filter("NEFRec < 1. && ZgMeasured > 0").Histo2D(zgdistmodel, "PtJetRec" , "ZgMeasured");
        auto distMg = jetsubstructureFrame.Filter("NEFRec < 1. && ZgMeasured > 0").Histo2D(mgdistmodel, "PtJetRec", "MgMeasured");
        auto distM = jetsubstructureFrame.Filter("NEFRec < 1.").Histo2D(mdistmodel, "PtJetRec", "MassRec"); 
        auto distAng = jetsubstructureFrame.Filter("NEFRec < 1.").Histo2D(angularitymodel, "PtJetRec", "AngularityMeasured");
        auto distPtd = jetsubstructureFrame.Filter("NEFRec < 1.").Histo2D(ptdmodel, "PtJetRec", "PtDMeasured");
    
        std::unique_ptr<TFile> writer(TFile::Open("structureDistributions_" + tag + ".root", "RECREATE"));
        distZg->Write();
        distMg->Write();
        distM->Write();
        distAng->Write();
        distPtd->Write();
    } else {
        auto distZg = jetsubstructureFrame.Filter("NEFRec < 1. && ZgMeasured > 0").Histo2D(zgdistmodel, "PtJetRec" , "ZgMeasured", "PythiaWeight");
        auto distMg = jetsubstructureFrame.Filter("NEFRec < 1. && ZgMeasured > 0").Histo2D(mgdistmodel, "PtJetRec", "MgMeasured", "PythiaWeight");
        auto distM = jetsubstructureFrame.Filter("NEFRec < 1.").Histo2D(mdistmodel, "PtJetRec", "MassRec", "PythiaWeight"); 
        auto distAng = jetsubstructureFrame.Filter("NEFRec < 1.").Histo2D(angularitymodel, "PtJetRec", "AngularityMeasured", "PythiaWeight");
        auto distPtd = jetsubstructureFrame.Filter("NEFRec < 1.").Histo2D(ptdmodel, "PtJetRec", "PtDMeasured", "PythiaWeight");
    
        std::unique_ptr<TFile> writer(TFile::Open("structureDistributions_" + tag + ".root", "RECREATE"));
        distZg->Write();
        distMg->Write();
        distM->Write();
        distAng->Write();
        distPtd->Write();
    }
}