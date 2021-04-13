#include "../meta/stl.C"
#include "../meta/root.C"

struct ptbin {
    int ptmin;
    int ptmax;
    TH1 *hZg;
    TH1 *hRg;
    TH1 *hThetag;
    TH1 *hNsd;

    bool operator<(const ptbin &other) const { return ptmax <= other.ptmin; }
    bool operator==(const ptbin &other) const { return ptmin == other.ptmin && ptmax == other.ptmax; }

    void Write(const std::string_view probe) {
        if(probe == "Zg") hZg->Write();
        else if(probe == "Rg") hRg->Write();
        else if(probe == "Thetag") hThetag->Write();
        else if(probe == "Nsd") hNsd->Write();
    }
};

std::vector<ptbin> project(TH2 &hist, int R, const std::string_view observable){
    std::cout << "Project observable " << observable << std::endl;
    const double kVerySmall = 1e-5;
    std::vector<double> ptbinning = {15, 20, 30, 40, 50, 60, 80, 100, 120, 140, 160, 180, 200}; 

    std::vector<ptbin> result;
    for(auto ib : ROOT::TSeqI(0, ptbinning.size()-1)) {
        auto binmin = hist.GetYaxis()->FindBin(ptbinning[ib] + kVerySmall),
             binmax = hist.GetYaxis()->FindBin(ptbinning[ib+1] - kVerySmall);
        auto spectrum = hist.ProjectionX(Form("h%s_R%20d_%d_%d", observable.data(), R, static_cast<int>(ptbinning[ib]), static_cast<int>(ptbinning[ib+1])), binmin, binmax, "e");
        spectrum->SetDirectory(nullptr);
        spectrum->Scale(1./spectrum->Integral());
        spectrum->Scale(1., "width");
        ptbin nextbin{static_cast<int>(ptbinning[ib]), static_cast<int>(ptbinning[ib+1]), nullptr, nullptr, nullptr, nullptr};
        if(observable == "Zg") nextbin.hZg = spectrum;
        else if(observable == "Rg") nextbin.hRg = spectrum;
        else if(observable == "Thetag") nextbin.hThetag = spectrum;
        else if(observable == "Nsd") nextbin.hNsd = spectrum;
        result.push_back(nextbin);
    }
    return result;
}

std::vector<ptbin> projectNsd(TH2 &hist, int R, const std::string_view observable){
    std::vector<double> nsdbins;
    nsdbins.push_back(hist.GetXaxis()->GetBinLowEdge(1));
    for(auto ib : ROOT::TSeqI(0, hist.GetXaxis()->GetNbins())) {
        auto binmax = hist.GetXaxis()->GetBinUpEdge(ib+1);
        if(binmax > 10.) break;
        nsdbins.push_back(binmax);
    }
    std::vector<double> ptbins;
    ptbins.push_back(hist.GetYaxis()->GetBinLowEdge(1));
    for(auto ib : ROOT::TSeqI(0, hist.GetYaxis()->GetNbins())) {
        auto binmax = hist.GetYaxis()->GetBinUpEdge(ib+1);
        ptbins.push_back(binmax);
    }

    std::unique_ptr<TH2> nsdhist(new TH2D(Form("%s_tmp", hist.GetName()), hist.GetTitle(), nsdbins.size()-1, nsdbins.data(), ptbins.size()-1, ptbins.data()));
    nsdhist->Sumw2();
    for(auto xb : ROOT::TSeqI(0, nsdhist->GetXaxis()->GetNbins())) {
        for(auto yb : ROOT::TSeqI(0, nsdhist->GetYaxis()->GetNbins())) {
            nsdhist->SetBinContent(xb+1, yb+1, hist.GetBinContent(xb+1, yb+1));
            nsdhist->SetBinError(xb+1, yb+1, hist.GetBinError(xb+1, yb+1));
        }
    }
    return project(*nsdhist, R, "Nsd");
}

std::vector<ptbin> readDistsR(TFile &reader, int R) {
    std::cout << "Doing R " << R << std::endl;
    reader.cd(Form("R%02d", R));

    auto dataZg = project(*static_cast<TH2 *>(gDirectory->Get(Form("hZgAbsR%02d", R))), R, "Zg"),
         dataRg = project(*static_cast<TH2 *>(gDirectory->Get(Form("hRgAbsR%02d", R))), R, "Rg"),
         dataThetag = project(*static_cast<TH2 *>(gDirectory->Get(Form("hThetagAbsR%02d", R))), R, "Thetag"),
         dataNsd = projectNsd(*static_cast<TH2 *>(gDirectory->Get(Form("hNsdAbsR%02d", R))), R, "Nsd");

    std::vector<ptbin> result;
    for(auto zb : dataZg) {
        auto binRg = std::find(dataRg.begin(), dataRg.end(), zb),
             binThetag = std::find(dataThetag.begin(), dataThetag.end(), zb),
             binNsd = std::find(dataNsd.begin(), dataNsd.end(), zb);
        ptbin combined{zb.ptmin, zb.ptmax, zb.hZg, binRg->hRg, binThetag->hThetag, binNsd->hNsd};
        result.push_back(combined);
    }
    return result;
}

void extractMCTrueHerwig(const char *inputfile = "CombinedSD.root") {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ"));
    std::map<int, std::vector<ptbin>> data;
    for(auto R : ROOT::TSeqI(2, 7)) data[R] = readDistsR(*reader, R);

    std::unique_ptr<TFile> writer(TFile::Open("herwigSoftDrop.root", "RECREATE"));
    std::vector<std::string> observables = {"Zg", "Rg", "Thetag", "Nsd"};
    for(auto obs : observables) {
        writer->mkdir(obs.data());
        writer->cd(obs.data());
        auto basedir = gDirectory;
        for(auto [R, ptb] : data) {
            basedir->mkdir(Form("R%02d", R));
            basedir->cd(Form("R%02d", R));
            for(auto ib : ptb) {
                std::cout << "Writing ptbin from " << ib.ptmin << " to " << ib.ptmax << std::endl;
                ib.Write(obs);
            } 
        }
    }
}