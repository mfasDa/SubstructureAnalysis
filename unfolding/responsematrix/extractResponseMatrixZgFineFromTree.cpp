#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/pthard.C"
#include "../../helpers/substructuretree.C"
#include "../../helpers/string.C"
#include "../binnings/binningZg.C"

void extractResponseMatrixZgFineFromTree(const std::string_view treefile){
    ROOT::EnableImplicitMT(8);
    auto tag = getFileTag(treefile);
    auto jd = getJetType(tag);
    ROOT::RDataFrame dataframe(GetNameJetSubstructureTree(treefile), treefile);
    auto binningpart = getPtBinningPart(jd.fTrigger),
         binningdet  = getPtBinningRealistic(jd.fTrigger);

    auto ptsmearmin = *binningdet.begin(), ptsmearmax = *binningdet.rbegin();
    std::vector<ROOT::RDF::RResultPtr<TH2D>> responsematrices;
    auto rejectoutlier = dataframe.Define("Outlier", [](double ptsim, int pthardbin) { if(IsOutlier(ptsim, pthardbin)) return 1.; else return 0.; }, {"PtJetSim", "PtHardBin"}).Filter("Outlier < 1.");
    auto responsematrixall = rejectoutlier.Filter(Form("PtJetRec >= %.1f && PtJetRec < %.1f", ptsmearmin, ptsmearmax)).Histo2D({"matrixfine_allptsim", "; z_{g,det}; z_{g,part}", 55, 0., 0.55, 55, 0., 0.55}, "ZgMeasured", "ZgTrue", "PythiaWeight");
    for(auto irange : ROOT::TSeqI(0, binningpart.size() - 1)) {
        auto histptr = rejectoutlier.Filter(Form("PtJetRec >= %.1f && PtJetRec < %.1f && PtJetSim >= %1f && PtJetSim < %.1f", ptsmearmin, ptsmearmax, binningpart[irange], binningpart[irange+1])).Histo2D({Form("matrixfine_%d_%d", int(binningpart[irange]), int(binningpart[irange+1])), "; z_{g,det}; z_{g,part}", 55, 0., 0.55, 55, 0., 0.55}, "ZgMeasured", "ZgTrue", "PythiaWeight");
        responsematrices.push_back(histptr);
    }
    // Write to file
    std::unique_ptr<TFile> writer(TFile::Open(Form("responsematrixZg_%s.root", tag.data()), "RECREATE"));
    responsematrixall->Write();
    for(auto slice : responsematrices) slice->Write();
}