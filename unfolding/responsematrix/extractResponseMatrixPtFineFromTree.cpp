#include "../../meta/root.C"
#include "../../helpers/pthard.C"
#include "../../helpers/substructuretree.C"

void extractResponseMatrixPtFineFromTree(const std::string_view treefile){
    ROOT::EnableImplicitMT(8);
    auto tag = getFileTag(treefile);
    auto jd = getJetType(tag);
    ROOT::RDataFrame dataframe(GetNameJetSubstructureTree(treefile), treefile);
    auto rejectoutlier = dataframe.Define("Outlier", [](double ptsim, int pthardbin) { if(IsOutlierFast(ptsim, pthardbin)) return 1.; else return 0.; }, {"PtJetSim", "PtHardBin"}).Filter("Outlier < 1.");
    auto responsematrix = rejectoutlier.Histo2D({"matrixfine_allptsim", "; p_{t,part} (GeV/c); p_{t,det} (GeV/c)", 180, 20., 200., 190, 10., 200.}, "PtJetSim", "PtJetRec", "PythiaWeight");
    auto diffmatrix =  rejectoutlier.Define("PtJetDiff", "(PtJetRec - PtJetSim)/PtJetSim").Histo2D({"ptdiff_allptsim", "; p_{t,part} (GeV/c); p_{t,det} (GeV/c)", 200, 20., 200., 200, -1., 1.}, "PtJetSim", "PtJetDiff", "PythiaWeight");

    TGraphErrors *mean = new TGraphErrors, *median = new TGraphErrors, *sigma = new TGraphErrors;
    for(auto b : ROOT::TSeqI(0, diffmatrix->GetXaxis()->GetNbins())) {
        std::unique_ptr<TH1> slice(diffmatrix->ProjectionY("py", b+1, b+1));
        mean->SetPoint(b, diffmatrix->GetXaxis()->GetBinCenter(b+1), slice->GetMean());
        mean->SetPointError(b, diffmatrix->GetXaxis()->GetBinWidth(b+1)/2., slice->GetMeanError()); 
        sigma->SetPoint(b, diffmatrix->GetXaxis()->GetBinCenter(b+1), slice->GetRMS());
        sigma->SetPointError(b, diffmatrix->GetXaxis()->GetBinWidth(b+1)/2., slice->GetRMSError()); 
        double vmedian, quantile(0.5);
        slice->GetQuantiles(1, &vmedian, &quantile);
        median->SetPoint(b, diffmatrix->GetXaxis()->GetBinCenter(b+1), vmedian);
        median->SetPointError(b, diffmatrix->GetXaxis()->GetBinWidth(b+1)/2., 0);
    }

    std::unique_ptr<TFile> writer(TFile::Open(Form("responsematrixpt_%s.root", tag.data()), "RECREATE"));
    writer->cd();
    responsematrix->Write();
    diffmatrix->Write();
    mean->Write("mean");
    sigma->Write("sigma");
    median->Write("median");
}