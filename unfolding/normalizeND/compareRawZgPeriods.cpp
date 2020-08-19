#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

const double kVerySmall = 1e-5;

bool IsEqual(double v1, double v2) { return TMath::Abs(v1-v2) < kVerySmall; }

struct PtBin {
    double ptmin;
    double ptmax;
    TH1 *dist;

    bool operator==(const PtBin &other) const {
        return IsEqual(ptmin, other.ptmin) && IsEqual(ptmax, other.ptmax);
    }

    bool operator<(const PtBin &other) const {
        return ptmax <= other.ptmin;
    }
};

std::map<int, std::vector<PtBin>> readFile(const char *filename, const char *period, const char *observable) {
    std::cout << "Reading " << period << std::endl;
    std::vector<double> ptlimits = {100., 120., 140., 160., 180, 200.};
    std::map<int, std::vector<PtBin>> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    
    for(auto R : ROOT::TSeqI(2, 7)) {
        reader->cd(Form("SoftDropResponse_FullJets_R%02d_EJ1", R));
        auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto eventcounter = static_cast<TH1 *>(histos->FindObject("hEventCounter"));
        auto rawzg = static_cast<TH2 *>(histos->FindObject(Form("h%sVsPt", observable)));
        std::vector<PtBin> dists;
        for(int ipt = 0; ipt < ptlimits.size() - 1; ipt++) {
            auto ptmin = ptlimits[ipt], ptmax = ptlimits[ipt+1];
            auto binmin = rawzg->GetYaxis()->FindBin(ptmin + kVerySmall), binmax = rawzg->GetYaxis()->FindBin(ptmax - kVerySmall);
            auto hslice = rawzg->ProjectionX(Form("h%s_%s_R%02d_%d_%d", observable, period, R, int(ptmin), int(ptmax)), binmin, binmax);
            hslice->SetDirectory(nullptr);
            hslice->Scale(1./eventcounter->GetBinContent(1));
            dists.push_back({ptmin, ptmax, hslice});
        }
        std::sort(dists.begin(), dists.end(), std::less<PtBin>());
        result[R] = dists;
    }

    return result;
}

void compareRawZgPeriods(const char *observable){
    std::vector<std::string> periods2017 = {"LHC17h", "LHC17i", "LHC17j", "LHC17k", "LHC17l", "LHC17m", "LHC17o", "LHC17r"},
                             periods2018 = {"LHC18d", "LHC18e", "LHC18f", "LHC18ghijk", "LHC18l", "LHC18m", "LHC18no", "LHC18p"};

    std::map<std::string, std::map<int, std::vector<PtBin>>> data;
    for(auto period : periods2017) data[period] = readFile(Form("2017/%s/AnalysisResults.root", period.data()), period.data(), observable);
    for(auto period : periods2018) data[period] = readFile(Form("2018/%s/AnalysisResults.root", period.data()), period.data(), observable);

    std::map<std::string, Color_t> periodcolors;
    std::map<std::string, Style_t> periodmarkers;

    std::vector<Color_t> colors = {kRed, kBlue, kGreen, kOrange, kViolet, kTeal, kMagenta, kGray};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28};
    
    int icol = 0, imrk = 0;
    for(auto per : periods2017) {
        periodcolors[per] = colors[icol];
        periodmarkers[per] = markers[imrk];
        icol++;
        imrk++;
        if(icol >= colors.size()) icol = 0;
        if(imrk >= markers.size()) imrk = 0;
    }
    for(auto per : periods2018) {
        periodcolors[per] = colors[icol];
        periodmarkers[per] = markers[imrk];
        icol++;
        imrk++;
        if(icol >= colors.size()) icol = 0;
        if(imrk >= markers.size()) imrk = 0;
    }

    auto style = [](Color_t col, Style_t mrk) {
        return [col, mrk] (auto obj) {
            obj->SetMarkerColor(col);
            obj->SetMarkerStyle(mrk);
            obj->SetLineColor(col);
        };
    };

    for(auto R : ROOT::TSeqI(2, 7)) {
        auto plot = new ROOT6tools::TSavableCanvas(Form("periodComparisonRaw%s_R%02d", observable, R), Form("Period comparison for raw %s for R = %.1f", observable, double(R)/10.), 1200, 800);
        plot->Divide(3,2);

        auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.5, 0.89);

        for(int ipt : ROOT::TSeqI(0,5)) {
            plot->cd(ipt+1);
            double ptmin, ptmax, xmin, xmax, ymax = 0;
            bool initialized = false;
            std::map<std::string, TH1 *> ptdata;
            // Read data for same pt bin from all samples
            for(const auto &[per, rdata] : data) {
                const auto &dists = rdata.find(R)->second;
                const auto &dist = dists[ipt];
                if(!initialized) {
                    ptmin = dist.ptmin;
                    ptmax = dist.ptmax;
                    xmin = dist.dist->GetXaxis()->GetBinLowEdge(1);
                    xmax = dist.dist->GetXaxis()->GetBinUpEdge(dist.dist->GetXaxis()->GetNbins());
                    initialized = true;
                }
                if(dist.dist->GetMaximum() > ymax) ymax = dist.dist->GetMaximum();
                ptdata[per] = dist.dist;
            }

            (new ROOT6tools::TAxisFrame(Form("specframe_%s_R%02d_%d_%d", observable, R, int(ptmin), int(ptmax)), observable, "1/N_{ev} dN/dz_{g}", xmin, xmax, 0, ymax))->Draw("axis");
            (new ROOT6tools::TNDCLabel(0.15, 0.9, 0.89, 0.95, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptmin, ptmax)))->Draw();
            for(const auto &[per, spec] : ptdata) {
                style(periodcolors[per], periodmarkers[per])(spec);
                spec->Draw("epsame");
                if(ipt == 0) leg->AddEntry(spec, per.data(), "lep");                
            }
        }

        plot->cd(6);
        leg->Draw();

        plot->cd();
        plot->Update();
        plot->SaveCanvas(plot->GetName());
    }
}