#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"

struct rdist {
    int fR;
    TH1 *fSpectrum = nullptr;
    TH2 *fZgVsPt = nullptr;
    TH2 *fRgVsPt = nullptr;
    TH2 *fThetagVsPt = nullptr;
    TH2 *fNsdVsPt = nullptr;

    bool operator==(const rdist &other) const { return fR == other.fR; }
    bool operator<(const rdist &other) const { return fR < other.fR; }

    void Write(TFile &output) {
        std::string rstring = Form("R%02d", fR);
        output.mkdir(rstring.data());
        output.cd(rstring.data());
        fSpectrum->Write();
        fZgVsPt->Write();
        fRgVsPt->Write();
        fThetagVsPt->Write();
        fNsdVsPt->Write();
    }
};

std::set<rdist> parseFile(const char *filename, int outliercut) {
    auto prepareHist = [](auto hist, double weight) {
        hist->SetDirectory(nullptr);
        hist->Scale(weight);
    };
    std::string outlierstring = outliercut < 0 ? "nooutlier" : Form("outlier%d", outliercut);
    std::set<rdist> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    for(auto R : ROOT::TSeqI(2, 7)){
        std::string rstring = Form("R%02d", R);
        auto rdir = "PartLevelJetResultsFullJet" + rstring + "_" + outlierstring;   
        std::cout << "Using rdir " << rdir << std::endl;
        auto histlist = reader->Get<TList>(rdir.data());
        if(!histlist) std::cout << "List of histograms not found" << std::endl;
        auto xsec = static_cast<TH1 *>(histlist->FindObject("fHistXsection"))->GetBinContent(1),
             ntrials = static_cast<TH1 *>(histlist->FindObject("fHistTrials"))->GetBinContent(1),
             weight = xsec / ntrials;
        auto spectrum = static_cast<TH1 *>(histlist->FindObject("hJetPt"));
        auto zgvspt = static_cast<TH2 *>(histlist->FindObject("hSDZg")),
             rgvspt = static_cast<TH2 *>(histlist->FindObject("hSDRg")),
             thetagvspt = static_cast<TH2 *>(histlist->FindObject("fSDThetag")),
             nsdvspt = static_cast<TH2 *>(histlist->FindObject("fSDNsd"));
        prepareHist(spectrum, weight);
        prepareHist(zgvspt, weight);
        prepareHist(rgvspt, weight);
        prepareHist(thetagvspt, weight);
        prepareHist(nsdvspt, weight);
        result.insert({R, spectrum, zgvspt, rgvspt, thetagvspt, nsdvspt});
    }
    return result;
}

std::vector<int> findKtMin(const std::string_view workdir) {
    std::unique_ptr<TObjArray> content(gSystem->GetFromPipe(Form("ls -1 %s", workdir.data())).Tokenize("\n"));
    std::vector<int> result;
    for(auto cnt : TRangeDynCast<TObjString>(content.get())) {
        if(cnt->String().IsDigit()) result.push_back(cnt->String().Atoi());
    }
    std::sort(result.begin(), result.end(), std::less<int>());
    return result;
}

void replace1D(TH1 *target, TH1 *source, int minpt) {
    for(int ib : ROOT::TSeqI(target->GetXaxis()->FindBin(minpt + 1e-5), target->GetXaxis()->GetNbins()+1)){
        target->SetBinContent(ib, source->GetBinContent(ib));
        target->SetBinError(ib, source->GetBinError(ib));
    }
}

void replace2D(TH2 *target, TH2 *source, int minpt) {
    for(int yb : ROOT::TSeqI(target->GetYaxis()->FindBin(minpt + 1e-5), target->GetYaxis()->GetNbins()+1)){
        for(int xb : ROOT::TSeqI(1, target->GetXaxis()->GetNbins()+1)){
            target->SetBinContent(xb, yb, source->GetBinContent(xb, yb));
            target->SetBinError(xb, yb, source->GetBinError(xb, yb));
        }
    }
}

void makeCombinedSpectrumSD_mbkthard(){
    std::map<int, std::set<rdist>> histosKt;
    for(auto minkt : findKtMin(gSystem->GetWorkingDirectory())) {
        if(minkt < 20) continue;
        std::cout << "Processing min. kt " << minkt << std::endl; 
        histosKt[minkt] = parseFile(Form("%d/AnalysisResults.root", minkt), -1);
    }
    auto histosMinBias = parseFile("mb/AnalysisResults.root", -1);

    std::map<int, int> minktmap = {{20., 50.}, {50, 90,}, {90, 150}, {150, 250}, {230, 400}};

    for(auto [minkt, histos] : histosKt) {
        auto minpt = minktmap[minkt];
        std::cout << "Using data from kt-min for pt > " << minpt << " GeV/c" << std::endl;
        for(auto R : ROOT::TSeqI(2, 7)){
            auto rhistsMB = histosMinBias.find({R}),
                 rhistsKt = histos.find({R});
            replace1D(rhistsMB->fSpectrum, rhistsKt->fSpectrum, minpt);
            replace2D(rhistsMB->fZgVsPt, rhistsKt->fZgVsPt, minpt);
            replace2D(rhistsMB->fRgVsPt, rhistsKt->fRgVsPt, minpt);
            replace2D(rhistsMB->fThetagVsPt, rhistsKt->fThetagVsPt, minpt);
            replace2D(rhistsMB->fNsdVsPt, rhistsKt->fNsdVsPt, minpt);
        }
    }
    std::unique_ptr<TFile> writer(TFile::Open("CombinedSD.root", "RECREATE"));
    for(auto bin : histosMinBias) bin.Write(*writer);
}