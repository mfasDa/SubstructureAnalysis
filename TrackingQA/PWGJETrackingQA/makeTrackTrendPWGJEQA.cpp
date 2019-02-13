#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/string.C"
#include "../../helpers/math.C"

struct ptrange {
    double fPtMin;
    double fPtMax;

    bool operator==(const ptrange &other) const { return fPtMin == other.fPtMin && fPtMax == other.fPtMax; }
    bool operator<(const ptrange &other) const { return fPtMax <= other.fPtMin; }
};

TH1 *getSpectrum(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto histos = static_cast<TH1 *>(reader->Get("AliEmcalTrackingQATask_histos"));
    auto hTracks = static_cast<THnSparse *>(histos->FindObject("fTracks"));
    auto hNorm = static_cast<TH1 *>(histos->FindObject("fHistEventCount"));
    auto norm = hNorm->GetBinContent(hNorm->GetXaxis()->FindBin("Accepted"));
    auto spectrum = hTracks->Projection(0);
    spectrum->SetName("spectrum");
    spectrum->SetDirectory(nullptr);
    normalizeBinWidth(spectrum);
    spectrum->Scale(1./norm);
    return spectrum;
}

std::vector<std::string> getListOfPeriods(const std::string_view perioddir){
    return tokenize(gSystem->GetFromPipe(Form("ls -1 %s | grep LHC", perioddir.data())).Data(), '\n');
}

void makeTrackTrendPWGJEQA(){
    std::string basedir = gSystem->GetWorkingDirectory();
    std::map<std::string, TH1 *> spectra;
    std::array<int, 3> years = {{2016, 2017, 2018}};
    std::array<ptrange, 6> ptsteps = {{{0.5, 0.6}, {1., 1.2} , {2., 2.4}, {5., 6.}, {10., 14.}, {20., 30.}}};
    for(auto y : years){
        std::cout << "Processing year " << y << std::endl;
        std::stringstream yeardir;
        yeardir << basedir << "/" << y;
        for(const auto &p : getListOfPeriods(yeardir.str())){
            std::cout << "Found period " << p.data() << std::endl;
            std::stringstream rootfile;
            rootfile << yeardir.str() << "/" << p << "/AnalysisResults.root";
            auto spec = getSpectrum(rootfile.str());
            spec->SetName(Form("%s_%s", spec->GetName(), p.data()));
            spectra[p] =  spec;
        }
    }

    std::vector<TH1 *> trends;
    for(auto pt : ptsteps){
        auto trendgraph = new TH1F(Form("trend_hybrid_%d_%d", int(pt.fPtMin * 10.), int(pt.fPtMax * 10.)), Form("Trend hybrid tracks, %.1f GeV/c p_{t} < %.1f GeV/c", pt.fPtMin, pt.fPtMax), spectra.size(), -0.5, spectra.size() - 0.5);
        trendgraph->SetDirectory(nullptr);
        trends.emplace_back(trendgraph);
        int ibin = 1;
        for(const auto &s : spectra){
            auto ptbinmin = s.second->GetXaxis()->FindBin(pt.fPtMin + 1e-5),
                 ptbinmax = s.second->GetXaxis()->FindBin(pt.fPtMax - 1e-5);
            trendgraph->GetXaxis()->SetBinLabel(ibin, s.first.data());
            double ig, err;
            ig = s.second->IntegralAndError(ptbinmin, ptbinmax, err);
            trendgraph->SetBinContent(ibin, ig);
            trendgraph->SetBinError(ibin, err);
            ibin++;
        }
    }

    std::unique_ptr<TFile> reader(TFile::Open("trendTracking_hybrid_MB_full.root", "RECREATE"));
    for(auto t : trends) t->Write();
}