#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/cdb.C"

std::map<std::string, TGraph*> readTrends(const char *filename) {
    std::map<std::string, TGraph*> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    reader->cd("RelFilter");
    std::array<std::string, 3> acceptance = {{"All", "EMCAL", "DCAL"}};
    for(const auto &acc : acceptance) {
        auto graph = static_cast<TGraph *>(gDirectory->Get(Form("trendMaskL1%sRelFilter", acc.data())));
        result[acc] = graph;
    }
    gROOT->cd();
    return result;
}

struct plotrange {
    double xmin;
    double xmax;
};

plotrange getRunRangePlot() {
    double xmin = 265700, xmax = 267200;
    return {xmin, xmax};
};

void scalePercent(TGraph *trend) {
    for(auto pnt : ROOT::TSeqI(0, trend->GetN())) {
        trend->SetPointY(pnt, trend->GetPointY(pnt) * 100);
    }
}

void makeTrendingPlotpPb(const char *filename = "masktrending.root"){
    auto trends = readTrends(filename);
    auto runrange = getRunRangePlot();

    std::map<std::string, Color_t> colors = {{"All", kBlack}, {"EMCAL", kBlue}, {"DCAL", kRed}};
    std::map<std::string, std::string> legtitles = {{"All", "EMCal + DCal"}, {"EMCAL", "EMCal"}, {"DCAL", "DCal"}};
    std::array<std::string, 3> plotorder = {{"EMCAL", "DCAL", "All"}};

    auto plot = new ROOT6tools::TSavableCanvas("trendingMaskedFastorspPb", "Trend of the fraction of masked FastORs (p-Pb)", 800, 600);

    auto frame = new ROOT6tools::TAxisFrame("trendframe", "run number", "Masked FastORs (%)", runrange.xmin, runrange.xmax, 0., 50.);
    frame->Draw("axis");
    
    auto leg = new ROOT6tools::TDefaultLegend(0.6, 0.7, 0.89, 0.89);
    leg->Draw();
    
    for(const auto &trendtype : plotorder) {
        auto trend = trends[trendtype];
        scalePercent(trend);
        trend->SetLineColor(colors[trendtype]);
        trend->Draw("lsame");
        leg->AddEntry(trend, legtitles[trendtype].data(), "l");
    }
    
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}