#include "../helpers/graphics.C"
#include "../helpers/math.C"
#include "../helpers/string.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../meta/stl.C"

struct CorrectedSpectrum {
    std::string fPeriod;
    TH1 *fSpectrum;

    bool operator<(const CorrectedSpectrum &other) const { return fPeriod < other.fPeriod; }
    bool operator==(const CorrectedSpectrum &other) const { return fPeriod == other.fPeriod; }
};

struct Trendgraph {
    double fPtMin;
    double fPtMax;
    TH1 *fHist;

    bool operator<(const Trendgraph &other) const { return fPtMax <= other.fPtMin; }
    bool operator==(const Trendgraph &other) const { return fPtMin == other.fPtMin && fPtMax == other.fPtMax; }
};

TH1 *readEfficiency(const std::string_view filename) {
    TH1 *result(nullptr);
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    result = static_cast<TH1 *>(reader->Get("efficiency"));
    result->SetDirectory(nullptr);
    return result;
}

TH1 *readSpectrum(const std::string_view filename, const std::string_view tracktype, const std::string_view acceptance) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd(Form("ChargedParticleQA_%s_nocorr", tracktype.data()));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    std::unique_ptr<THnSparse> specND(static_cast<THnSparse *>(histlist->FindObject("hPtEtaPhiAllMB")));
    auto norm = static_cast<TH1 *>(histlist->FindObject("hEventCountMB"));
    if(acceptance == "EMCAL") {
        specND->GetAxis(1)->SetRangeUser(-0.6, 0.6);
        specND->GetAxis(2)->SetRangeUser(1.4, 3.1);
    }
    auto projected = specND->Projection(0);
    projected->SetDirectory(nullptr);
    projected->Scale(1./norm->GetBinContent(1));
    normalizeBinWidth(projected);
    return projected;
}

std::set<std::string> getListOfDataPeriods(const std::string_view inputdir){
    std::string dirstring = gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data();
    std::set<std::string> directories;
    for(const auto &d : tokenize(dirstring, '\n')) directories.insert(d);
    return directories;
}

void makeTrackingComparisonCorrected(const std::string_view tracktype = "hybrid", const std::string_view acceptance = "full") {
    std::set<CorrectedSpectrum> spectra;
    for(const auto &p : getListOfDataPeriods("Data")){
        std::cout << "Reading " << p << " ... " << std::endl;
        auto eff = readEfficiency(Form("MC/%s/effTracking_%s_MB_%s.root", p.data(), tracktype.data(), acceptance.data())),
             spec = readSpectrum(Form("Data/%s/AnalysisResults.root", p.data()), tracktype, acceptance);
        spec->SetName(p.data());
        spec->Divide(eff);
        delete eff;
        spectra.insert({p, spec});
    }

    const std::array<Color_t, 10> colors = {{kRed, kBlue, kGreen, kMagenta, kOrange, kTeal, kGray, kOrange, kViolet, kAzure}};
    const std::array<Style_t, 8> markers = {{24, 25, 26, 27, 28, 29, 30, 31}};

    auto plot = new ROOT6tools::TSavableCanvas(Form("ComparisonSpectraCorrected_%s_%s", tracktype.data(), acceptance.data()), "Comparsion corrected Spectra", 800, 600);
    plot->SetLogy();
    (new ROOT6tools::TAxisFrame("spectraframe", "p_{t} (GeV/c)", "1/N_{ev} dN/dp{t} ((GeV/c)^{-1})", 0., 100., 1e-9, 100.))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.35, 0.5, 0.89, 0.89);
    leg->SetNColumns(2);
    leg->Draw();
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("Track cuts: %s", tracktype.data())))->Draw();
    int icol(0), imrk(0);
    for(const auto &s : spectra) {
        Style{colors[icol++], markers[imrk++]}.SetStyle<TH1>(*s.fSpectrum);
        if(icol == 10) icol = 0;
        if(imrk == 8) imrk = 0;
        s.fSpectrum->Draw("epsame");
        leg->AddEntry(s.fSpectrum, s.fPeriod.data(), "lep");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());

    // Create trending graphs
    std::set<Trendgraph> trends;
    std::array<double, 6> ptmin = {{0.5, 1., 2., 5., 10., 20.}},
                          ptmax = {{0.6, 1.2, 2.4, 6., 15., 30.}};
    for(int i = 0; i < 6; i++){
        auto hist = new TH1D(Form("trend_%s_%d_%d", tracktype.data(), int(ptmin[i] * 10), int(ptmax[i]*10.)), Form("Track trending %s tracks, %.1f GeV/c < p_{t} < %.1f GeV/c", tracktype.data(), ptmin[i], ptmax[i]), spectra.size(), 0., spectra.size());
        hist->SetDirectory(nullptr);
        int jb = 1;
        for(const auto &p : spectra) hist->GetXaxis()->SetBinLabel(jb++, p.fPeriod.data());
        trends.insert({ptmin[i], ptmax[i], hist});
    }

    const double kVerySmall = 1e-5;
    for(const auto &s : spectra){
        auto spectrum  = s.fSpectrum;
        for(const auto &t : trends) {
            auto binmin = spectrum->GetXaxis()->FindBin(t.fPtMin+kVerySmall), binmax = spectrum->GetXaxis()->FindBin(t.fPtMax-kVerySmall);
            double norm = t.fPtMax - t.fPtMin;
            double val = 0, errsquare = 0;
            for(auto b = binmin; b <= binmax; b++) {
                double bw = spectrum->GetXaxis()->GetBinWidth(b);
                val += spectrum->GetBinContent(b) * bw;
                errsquare += TMath::Power(spectrum->GetBinError(b) * bw, 2);
            }
            auto trendhist = t.fHist;
            trendhist->SetBinContent(trendhist->GetXaxis()->FindBin(s.fPeriod.data()), val/norm);
            trendhist->SetBinError(trendhist->GetXaxis()->FindBin(s.fPeriod.data()), TMath::Sqrt(errsquare)/norm);
        }
    }

    auto trendplot = new ROOT6tools::TSavableCanvas(Form("trackTrendingPeriods_%s_%s", tracktype.data(), acceptance.data()), "Track trending periods", 1200, 800);
    trendplot->Divide(3,2);
    int ipad = 1;
    for(auto h : trends){
        trendplot->cd(ipad++);
        gPad->SetLeftMargin(0.14);
        gPad->SetRightMargin(0.06);
        auto hist = h.fHist;
        hist->GetYaxis()->SetTitle("1/N_{ev} dN/dp_{t} ((GeV/c)^{-1})");
        hist->SetStats(false);
        hist->SetMarkerColor(kBlack);
        hist->SetMarkerStyle(20);
        hist->SetLineColor(kBlack);
        hist->Draw("pe");
    }
    trendplot->cd();
    trendplot->Update();
    trendplot->SaveCanvas(trendplot->GetName());

    std::unique_ptr<TFile> writer(TFile::Open(Form("trendTrackingPeriods_%s_%s.root", tracktype.data(), acceptance.data()), "RECREATE"));
    for(auto h : trends) h.fHist->Write();
}