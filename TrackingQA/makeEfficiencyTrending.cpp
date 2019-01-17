#include "../helpers/graphics.C"
#include "../helpers/math.C"
#include "../helpers/string.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../meta/stl.C"

struct Efficiency {
    std::string fPeriod;
    TH1 *fSpectrum;

    bool operator<(const Efficiency &other) const { return fPeriod < other.fPeriod; }
    bool operator==(const Efficiency &other) const { return fPeriod == other.fPeriod; }
};

struct Trendgraph {
    double fPtMin;
    TH1 *fHist;

    bool operator<(const Trendgraph &other) const { return fPtMin <= other.fPtMin; }
    bool operator==(const Trendgraph &other) const { return fPtMin == other.fPtMin; }
};

TH1 *readEfficiency(const std::string_view filename) {
    TH1 *result(nullptr);
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    result = static_cast<TH1 *>(reader->Get("efficiency"));
    result->SetDirectory(nullptr);
    return result;
}

std::set<std::string> getListOfDataPeriods(const std::string_view inputdir){
    std::string dirstring = gSystem->GetFromPipe(Form("ls -1 %s | grep LHC", inputdir.data())).Data();
    std::set<std::string> directories;
    for(const auto &d : tokenize(dirstring, '\n')) directories.insert(d);
    return directories;
}

void makeEfficiencyTrending(const std::string_view tracktype = "hybrid") {
    std::set<Efficiency> efficiencies;
    for(const auto &p : getListOfDataPeriods(gSystem->pwd())){
        std::cout << "Reading " << p << " ... " << std::endl;
        auto eff = readEfficiency(Form("%s/effTracking_%s_MB.root", p.data(), tracktype.data()));
        efficiencies.insert({p, eff});
    }

    const std::array<Color_t, 10> colors = {{kRed, kBlue, kGreen, kMagenta, kOrange, kTeal, kGray, kOrange, kViolet, kAzure}};
    const std::array<Style_t, 8> markers = {{24, 25, 26, 27, 28, 29, 30, 31}};

    auto plot = new ROOT6tools::TSavableCanvas("ComparisonTrackingEfficiency", "Comparsion tracking efficiency", 800, 600);
    (new ROOT6tools::TAxisFrame("spectraframe", "p_{t} (GeV/c)", "Tracking efficiency", 0., 100., 0, 1.))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.35, 0.15, 0.89, 0.5);
    leg->SetNColumns(2);
    leg->Draw();
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.35, 0.87, Form("Track cuts: %s", tracktype.data())))->Draw();
    int icol(0), imrk(0);
    for(const auto &e : efficiencies) {
        Style{colors[icol++], markers[imrk++]}.SetStyle<TH1>(*e.fSpectrum);
        if(icol == 10) icol = 0;
        if(imrk == 8) imrk = 0;
        e.fSpectrum->Draw("epsame");
        leg->AddEntry(e.fSpectrum, e.fPeriod.data(), "lep");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());

    // Create trending graphs
    std::set<Trendgraph> trends;
    std::array<double, 6> ptmin = {{0.5, 1., 2., 5., 10., 20.}};
    for(int i = 0; i < 6; i++){
        auto hist = new TH1D(Form("trend_%s_%d", tracktype.data(), int(ptmin[i] * 10)), Form("Track trending %s tracks, p_{t} = %.1f GeV/c", tracktype.data(), ptmin[i]), efficiencies.size(), 0., efficiencies.size());
        hist->SetDirectory(nullptr);
        int jb = 1;
        for(const auto &p : efficiencies) hist->GetXaxis()->SetBinLabel(jb++, p.fPeriod.data());
        trends.insert({ptmin[i], hist});
    }

    const double kVerySmall = 1e-5;
    for(const auto &e : efficiencies){
        auto spectrum  = e.fSpectrum;
        for(const auto &t : trends) {
            auto trendhist = t.fHist;
            auto bin = spectrum->GetXaxis()->FindBin(t.fPtMin + kVerySmall);
            trendhist->SetBinContent(trendhist->GetXaxis()->FindBin(e.fPeriod.data()), spectrum->GetBinContent(bin));
            trendhist->SetBinError(trendhist->GetXaxis()->FindBin(e.fPeriod.data()), spectrum->GetBinError(bin));
        }
    }

    auto trendplot = new ROOT6tools::TSavableCanvas("efficiencyTrendingPeriods", "Efficiency trending periods", 1200, 800);
    trendplot->Divide(3,2);
    int ipad = 1;
    for(auto h : trends){
        trendplot->cd(ipad++);
        gPad->SetLeftMargin(0.14);
        gPad->SetRightMargin(0.06);
        auto hist = h.fHist;
        hist->GetYaxis()->SetTitle("Tracking efficiency");
        hist->SetStats(false);
        hist->SetMarkerColor(kBlack);
        hist->SetMarkerStyle(20);
        hist->SetLineColor(kBlack);
        hist->Draw("pe");
    }
    trendplot->cd();
    trendplot->Update();
    trendplot->SaveCanvas(trendplot->GetName());

    std::unique_ptr<TFile> writer(TFile::Open("efficiencyTrendingPeriods.root", "RECREATE"));
    for(auto h : trends) h.fHist->Write();
}