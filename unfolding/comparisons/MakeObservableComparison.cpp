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

std::tuple<int, double, double> GetBinning(const std::string_view observable) {
    int nbins(0); double min(-1.), max(-1.);
    if(observable.find("Zg") != std::string::npos){
        nbins = 5;
        min = 0.;
        max = 0.5;
    } else if(observable.find("Mg") != std::string::npos) {
        nbins = 200;
        min = -100.;
        max = 100;
    } else if(observable.find("Mass") != std::string::npos){
        nbins = 100;
        min = 0.;
        max = 100.;
    } else if(observable.find("Rg") != std::string::npos) {
        nbins = 10;
        min = 0.;
        max = 1.;
    } else if(observable.find("Ptg") != std::string::npos) {
        nbins = 200;
        min = 0;
        max = 200;
    } else if(observable.find("Angularity") != std::string::npos) {
        nbins = 40;
        min = 0;
        max = 0.4;
    } else if (observable.find("PtD") != std::string::npos){
        nbins = 10;
        min = 0;
        max = 1;
    } 
    return std::make_tuple(nbins, min, max);
}

std::tuple<std::string, std::string, double, double> GetFrameConfig(const std::string_view observable){
    // Attention: Mu and Rg were swapped in code - fix after new version is available
    std::string xtitle, ytitle;
    double xmin, xmax;
    if(observable.find("Zg") != std::string::npos){
        xtitle = "z_{g}";
        ytitle = "1/N_{jet} dN_{jet}/dz_{g}";
        xmin = 0;
        xmax = 0.7;
    } else if(observable.find("Mg") != std::string::npos) {
        xtitle = "M_{g}";
        ytitle = "1/N_{jet} dN_{jet}/dM_{g}";
        xmin = -100;
        xmax = 100.5;
        /*
        xtitle = "M_{g} (GeV/c^{2})";
        ytitle = "1/N_{jet} dN_{jet}/dM_{g} ((GeV/c)^{-2})";
        xmin = 0;
        xmax = 1.;
        */
    } else if(observable.find("Mass") != std::string::npos){
        xtitle = "M (GeV/c^{2})";
        ytitle = "1/N_{jet} dN_{jet}/dM ((GeV/c)^{-2})";
        xmin = 0.;
        xmax = 100.;
    } else if(observable.find("Rg") != std::string::npos) {
        xtitle = "R_{g}";
        ytitle = "1/N_{jet} dN_{jet}/dR_{g}";
        xmin = 0;
        xmax = 1.;
    } else if(observable.find("Ptg") != std::string::npos) {
        xtitle = "p_{t,g} (GeV/c)";
        ytitle = "1/N_{jet} dN_{jet}/dp_{t,g} ((GeV/c)^{-1})";
        xmin = 0.;
        xmax = 200.;
    } else if(observable.find("Angularity") != std::string::npos) {
        xtitle = "Angularity";
        ytitle = "1/N_{jet} dN_{jet}/dAngularity";
        xmin = 0;
        xmax = 0.4;
    } else if (observable.find("PtD") != std::string::npos){
        xtitle = "p_{t,D}";
        ytitle = "1/N_{jet} dN_{jet}/dp_{t,D}";
        xmin = 0;
        xmax = 1.;
    } 

    return std::make_tuple(xtitle, ytitle, xmin, xmax);
}


std::set<ObservableHistJetPtBin> AnalyseTree(const std::string_view filename, const std::string_view observable){
    const std::vector<std::tuple<double, double>> ptbins = {
        {20., 40.}, {40., 60.}, {60., 80.}, {80., 100.}, {100., 120.}, {120., 140.}, {140., 160.}, {160., 180}, {180., 200.}
    };  
    auto binning = GetBinning(observable);
    int nbins = std::get<0>(binning);
    double min = std::get<1>(binning), max = std::get<2>(binning);
    std::set<ObservableHistJetPtBin> result;
    using ROOT::Experimental::TDataFrame;
    TDataFrame datahandler("jetSubstructure", filename.data());
    for(auto ptbin : ptbins) {
        double ptmin = std::get<0>(ptbin),
               ptmax = std::get<1>(ptbin);
        auto hist = datahandler.Filter(Form("ZgMeasured > 0 && NEFRec < 1. && PtJetRec > %f && PtJetRec < %f", ptmin, ptmax)).Histo1D({Form("%sdist_%d_%d", observable.data(), int(ptmin), int(ptmax)), Form("%s template", observable.data()), nbins, min, max}, Form("%sMeasured", observable.data()));
        //auto hist = datahandler.Filter(Form("NEFRec < 1. && PtJetRec > %f && PtJetRec < %f", ptmin, ptmax)).Histo1D({Form("%sdist_%d_%d", observable.data(), int(ptmin), int(ptmax)), Form("%s template", observable.data()), nbins, min, max}, Form("%sRec", observable.data()));
        ObservableHistJetPtBin mybin = {ptmin, ptmax, new TH1F(*hist)};
        mybin.resulthist->SetNameTitle(
            Form("%sdist_%d_%d", observable.data(), int(ptmin), int(ptmax)), 
            Form("%s-distribution from jets for %.1f GeV/c < p_{t, jet} < %.1f GeV/c", observable.data(), ptmin, ptmax));
        mybin.resulthist->SetDirectory(nullptr);
        mybin.resulthist->Scale(1./mybin.resulthist->Integral());
        result.insert(mybin);
    }
    return result;
}

void MakeObservableComparison(std::string_view observable) {
    ROOT::EnableImplicitMT();
    const std::array<std::string, 3> TRIGGERS = {"INT7", "EJ1", "EJ2"};
    const std::array<Color_t, 3>  COLS = {kBlack, kRed, kBlue};
    const std::array<Style_t, 3>  MKRS = {20, 24, 25};
    auto frameconfig = GetFrameConfig(observable);

    TCanvas *plot = new TCanvas(Form("%splot", observable.data()), Form("%splot", observable.data()), 1200, 1000);
    plot->Divide(3, 3);

    TLegend *leg = new TLegend(0.7, 0.7, 0.94, 0.94);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    
    int itrg = 0;
    for(const auto &trg : TRIGGERS){
        std::stringstream filenamebuilder;
        filenamebuilder << "JetSubstructureTree_R04_" << trg << ".root";
        auto data = AnalyseTree(filenamebuilder.str(), observable);

        int ipad = 1;
        for(const auto &bin : data) {
            plot->cd(ipad);
            if(itrg == 0) {
                gPad->SetLeftMargin(0.13);
                gPad->SetRightMargin(0.04);
                gPad->SetTopMargin(0.04);
                TH1 *hframe = new TH1F(Form("hframe_%d_%d", int(bin.ptmin), int(bin.ptmax)), "", 100, std::get<2>(frameconfig), std::get<3>(frameconfig));
                hframe->SetXTitle(std::get<0>(frameconfig).data());
                hframe->SetYTitle(std::get<1>(frameconfig).data());
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