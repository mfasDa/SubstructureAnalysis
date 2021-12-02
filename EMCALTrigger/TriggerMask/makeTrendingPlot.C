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

int detectYear(const TGraph *data) {
    int runmin = data->GetX()[0],
        runmax = data->GetX()[data->GetN()-1];
    int yearmin = getYearForRunNumber(runmin),
        yearmax = getYearForRunNumber(runmax);
    if(yearmin == 2016 && yearmax == 2016) return 2016;
    if(yearmin == 2017 && yearmax == 2017) return 2017;
    if(yearmin == 2018 && yearmax == 2018) return 2018;
    if(yearmin == 2016 && yearmax == 2017) return 201617;
    if(yearmin == 2016 && yearmax == 2018) return 201618;
    if(yearmin == 2017 && yearmax == 2018) return 201718; 
    return -1;
}

struct plotrange {
    double xmin;
    double xmax;
};

plotrange getRunRangePlot(int year) {
    double xmin = 0, xmax = 300000;
    switch(year) {
        case 2016: xmin = 254000; xmax = 265000; break;
        case 2017: xmin = 271000; xmax = 283000; break;
        case 2018: xmin = 286000; xmax = 295000; break;
        case 201617: xmin = 245000; xmax = 283000; break;
        case 201618: xmin = 252000; xmax = 296000; break;
        case 201718: xmin = 260000; xmax = 296000; break;
    };
    return {xmin, xmax};
};

void scalePercent(TGraph *trend) {
    for(auto pnt : ROOT::TSeqI(0, trend->GetN())) {
        trend->SetPointY(pnt, trend->GetPointY(pnt) * 100);
    }
}

void makeTrendingPlot(const char *filename = "masktrending.root"){
    auto trends = readTrends(filename);
    auto year = detectYear(trends["All"]);
    auto runrange = getRunRangePlot(year);

    std::map<std::string, Color_t> colors = {{"All", kBlack}, {"EMCAL", kBlue}, {"DCAL", kRed}};
    std::map<std::string, std::string> legtitles = {{"All", "EMCal + DCal"}, {"EMCAL", "EMCal"}, {"DCAL", "DCal"}};
    std::array<std::string, 3> plotorder = {{"EMCAL", "DCAL", "All"}};

    auto plot = new ROOT6tools::TSavableCanvas(Form("trendingMaskedFastors%d", year), Form("Trend of the fraction of masked FastORs (%d)", year), 800, 600);

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