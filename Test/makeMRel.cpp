#if !defined(__CINT__)
#include <array>
#include <cfloat>
#include <climits>
#include <set>
#include <sstream>
#include <tuple>
#include <vector>
#include <TCanvas.h>
#include <TH1.h>
#include <TLegend.h>
#include <TMath.h>
#include <TPaveText.h>
#include <ROOT/TDataFrame.hxx>
#include <RStringView.h>
#endif

struct ObservableHistJetPtBin {
    double ptmin, ptmax;
    TH1F *resulthist;

    bool operator<(const ObservableHistJetPtBin &other) const {
        return ptmin < other.ptmin;
    }

    bool operator==(const ObservableHistJetPtBin &other) const {
        return TMath::Abs(ptmin - other.ptmin) < DBL_EPSILON && TMath::Abs(ptmax - other.ptmax) < DBL_EPSILON;
    }
}; 

std::set<ObservableHistJetPtBin> AnalyseTree(const std::string_view filename){
    const std::vector<std::tuple<double, double>> ptbins = {
        {20., 40.}, {40., 60.}, {60., 80.}, {80., 100.}, {100., 120.}, {120., 140.}, {140., 160.}, {160., 180}, {180., 200.}
    };  
    int nbins = 20;
    double min = 0., max = 1.;
    std::set<ObservableHistJetPtBin> result;
    using ROOT::Experimental::TDataFrame;
    TDataFrame datahandler("jetSubstructure", filename.data());
    for(auto ptbin : ptbins) {
        double ptmin = std::get<0>(ptbin),
               ptmax = std::get<1>(ptbin);
        auto hist = datahandler.Filter(Form("NEFRec < 1. && ZgMeasured > 0 && PtJetRec > %f && PtJetRec < %f", ptmin, ptmax)).Define("MRel", [](double mg, double ptg) { return mg/ptg; }, {"MgMeasured", "PtgMeasured" }).Histo1D({Form("MReldist_%d_%d", int(ptmin), int(ptmax)), "MRel template", nbins, min, max}, "MRel");
        ObservableHistJetPtBin mybin = {ptmin, ptmax, new TH1F(*hist)};
        mybin.resulthist->SetNameTitle(
            Form("MReldist_%d_%d", int(ptmin), int(ptmax)), 
            Form("Mrel-distribution from jets for %.1f GeV/c < p_{t, jet} < %.1f GeV/c", ptmin, ptmax));
        mybin.resulthist->SetDirectory(nullptr);
        mybin.resulthist->Scale(1./mybin.resulthist->Integral());
        result.insert(mybin);
    }
    return result;
}

void makeMRel() {
    ROOT::EnableImplicitMT();
    const std::array<std::string, 3> TRIGGERS = {"INT7", "EJ1", "EJ2"};
    const std::array<Color_t, 3>  COLS = {kBlack, kRed, kBlue};
    const std::array<Style_t, 3>  MKRS = {20, 24, 25};

    TCanvas *plot = new TCanvas("MRelplot", "MRelplot", 1200, 1000);
    plot->Divide(3, 3);

    TLegend *leg = new TLegend(0.7, 0.7, 0.94, 0.94);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    
    int itrg = 0;
    for(const auto &trg : TRIGGERS){
        std::stringstream filenamebuilder;
        filenamebuilder << "JetSubstructureTree_R04_" << trg << ".root";
        auto data = AnalyseTree(filenamebuilder.str());

        int ipad = 1;
        for(const auto &bin : data) {
            plot->cd(ipad);
            if(itrg == 0) {
                gPad->SetLeftMargin(0.13);
                gPad->SetRightMargin(0.04);
                gPad->SetTopMargin(0.04);
                TH1 *hframe = new TH1F(Form("hframe_%d_%d", int(bin.ptmin), int(bin.ptmax)), "", 100, 0., 1.);
                hframe->SetXTitle("m_{g} / p_{t,g}");
                hframe->SetYTitle("1/N_{jet} dN_{jet}/dm_{rel}");
                hframe->SetDirectory(nullptr);
                hframe->SetStats(false);
                hframe->GetYaxis()->SetRangeUser(0., 0.7);
                hframe->Draw("axis");
                TPaveText *label = new TPaveText(0.15, 0.8, 0.7, 0.94, "NDC");
                label->SetBorderSize(0);
                label->SetFillStyle(0);
                label->SetTextFont(42);
                label->AddText(Form("%.1f GeV/c < p_{t, jet} < %.1f GeV/c", bin.ptmin, bin.ptmax));
                label->Draw();
                if(ipad == 1) leg->Draw();
            }
            if(ipad == 1) leg->AddEntry(bin.resulthist, trg.c_str(), "lep");
            bin.resulthist->SetLineColor(COLS[itrg]);
            bin.resulthist->SetMarkerColor(COLS[itrg]);
            bin.resulthist->SetMarkerStyle(MKRS[itrg]);
            bin.resulthist->Draw("epsame");
            gPad->Update();
            ipad++;
        }
        itrg++;
    }
}