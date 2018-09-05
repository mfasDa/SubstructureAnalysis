#include "../../meta/root.C"
#include "../../helpers/pthard.C"
#include "../../helpers/substructuretree.C"

void extractResponseMatrixPtFineFromTree(const std::string_view treefile){
    ROOT::EnableImplicitMT(8);
    auto tag = getFileTag(treefile);
    auto jd = getJetType(tag);
    ROOT::RDataFrame dataframe(GetNameJetSubstructureTree(treefile), treefile);
    auto rejectoutlier = dataframe.Define("Outlier", [](double ptsim, int pthardbin) { if(IsOutlier(ptsim, pthardbin)) return 1.; else return 0.; }, {"PtJetSim", "PtHardBin"}).Filter("Outlier < 1.");
    auto responsematrix = rejectoutlier.Histo2D({"matrixfine_allptsim", "; p_{t,part} (GeV/c); p_{t,det} (GeV/c)", 200, 0., 200., 180, 0., 200.}, "ZgMeasured", "ZgTrue", "PythiaWeight");

    TGraphErrors *mean = new TGraphErrors, *median = new TGraphErrors, *sigma = new TGraphErrors;
    for(auto b : ROOT::TSeqI(0, responsematrix->GetXaxis()->GetNbins())) {
        std::unique_ptr<TH1> slice(responsematrix->ProjectionY("py", b+1, b+1));
        mean->SetPoint(b, responsematrix->GetXaxis()->GetBinCenter(b+1), slice->GetMean());
        mean->SetPointError(b, responsematrix->GetXaxis()->GetBinWidth(b+1)/2., slice->GetMeanError()); 
        sigma->SetPoint(b, responsematrix->GetXaxis()->GetBinCenter(b+1), slice->GetRMS());
        sigma->SetPointError(b, responsematrix->GetXaxis()->GetBinWidth(b+1)/2., slice->GetRMSError()); 
        double vmedian, quantile(0.5);
        slice->GetQuantiles(1, &vmedian, &quantile);
        median->SetPoint(b, responsematrix->GetXaxis()->GetBinCenter(b+1), vmedian);
        median->SetPointError(b, responsematrix->GetXaxis()->GetBinWidth(b+1)/2., 0);
    }

    std::unique_ptr<TFile> writer(TFile::Open(Form("responsematrixpt_%s.root", tag.data()), "RECREATE"));
    writer->cd();
    responsematrix->Write();
    mean->Write("mean");
    sigma->Write("sigma");
    median->Write("median");
}