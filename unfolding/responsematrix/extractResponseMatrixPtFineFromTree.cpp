#include "../../meta/root.C"
#include "../../helpers/pthard.C"
#include "../../helpers/substructuretree.C"

void extractResponseMatrixPtFineFromTree(const std::string_view treefile, bool withoutliercut = kTRUE){
    ROOT::EnableImplicitMT(8);
    auto tag = getFileTag(treefile);
    auto jd = getJetType(tag);
    ROOT::RDataFrame dataframe(GetNameJetSubstructureTree(treefile), treefile);
    std::vector<ROOT::RDF::RResultPtr<TH2D>> results;
    if(withoutliercut) {
        auto rejectoutlier = dataframe.Define("Outlier", [](double ptsim, int pthardbin) { if(IsOutlierFast(ptsim, pthardbin)) return 1.; else return 0.; }, {"PtJetSim", "PtHardBin"}).Filter("Outlier < 1.");
        results.push_back(rejectoutlier.Histo2D({"matrixfine_allptsim", "; p_{t,part} (GeV/c); p_{t,det} (GeV/c)", 180, 20., 200., 190, 10., 200.}, "PtJetSim", "PtJetRec", "PythiaWeight"));
        results.push_back(rejectoutlier.Define("PtJetDiff", "(PtJetRec - PtJetSim)/PtJetSim").Histo2D({"ptdiff_allptsim", "; p_{t,part} (GeV/c); p_{t,det} (GeV/c)", 200, 0., 200., 200, -1., 1.}, "PtJetSim", "PtJetDiff", "PythiaWeight"));
    } else {
        results.push_back(dataframe.Histo2D({"matrixfine_allptsim", "; p_{t,part} (GeV/c); p_{t,det} (GeV/c)", 180, 20., 200., 190, 10., 200.}, "PtJetSim", "PtJetRec", "PythiaWeight"));
        results.push_back(dataframe.Define("PtJetDiff", "(PtJetRec - PtJetSim)/PtJetSim").Histo2D({"ptdiff_allptsim", "; p_{t,part} (GeV/c); p_{t,det} (GeV/c)", 200, 0., 200., 200, -1., 1.}, "PtJetSim", "PtJetDiff", "PythiaWeight"));
    }

    TGraphErrors *mean = new TGraphErrors, *median = new TGraphErrors, *sigma = new TGraphErrors;
    for(auto b : ROOT::TSeqI(0, results[1]->GetXaxis()->GetNbins())) {
        std::unique_ptr<TH1> slice(results[1]->ProjectionY("py", b+1, b+1));
        mean->SetPoint(b, results[1]->GetXaxis()->GetBinCenter(b+1), slice->GetMean());
        mean->SetPointError(b, results[1]->GetXaxis()->GetBinWidth(b+1)/2., slice->GetMeanError()); 
        sigma->SetPoint(b, results[1]->GetXaxis()->GetBinCenter(b+1), slice->GetRMS());
        sigma->SetPointError(b, results[1]->GetXaxis()->GetBinWidth(b+1)/2., slice->GetRMSError()); 
        double vmedian, quantile(0.5);
        slice->GetQuantiles(1, &vmedian, &quantile);
        median->SetPoint(b, results[1]->GetXaxis()->GetBinCenter(b+1), vmedian);
        median->SetPointError(b, results[1]->GetXaxis()->GetBinWidth(b+1)/2., 0);
    }

    std::unique_ptr<TFile> writer(TFile::Open(Form("responsematrixpt_%s.root", tag.data()), "RECREATE"));
    writer->cd();
    for(auto r : results) r->Write();
    mean->Write("mean");
    sigma->Write("sigma");
    median->Write("median");
}