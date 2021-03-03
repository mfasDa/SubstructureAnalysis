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

std::set<rdist> parseFile(const char *filename) {
    std::cout << "Reading file " << filename << std::endl;
    auto prepareHist = [](auto hist, double weight) {
        hist->SetDirectory(nullptr);
        hist->Scale(weight);
    };
    std::set<rdist> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));

    auto neventhist = reader->Get<TH1>("hNevents"),
	     xsechist = reader->Get<TH1>("hXsection");
    auto weight = xsechist->GetBinContent(1)/neventhist->GetBinContent(1);
    std::map<std::string, TDirectory *> directories;
    std::vector<std::string> dirnames = {"Spectra", "Zg", "Rg", "Thetag", "Nsd"};
    for(auto dirname : dirnames) {
        reader->cd(dirname.data());
        directories[dirname] = gDirectory;
    }
    
    
    for(auto R : ROOT::TSeqI(2, 7)){
        auto spectrum = directories["Spectra"]->Get<TH1>(Form("JetSpectrumAbsR%02d", R));
        auto zgvspt = directories["Zg"]->Get<TH2>(Form("hZgAbsR%02d", R)),
             rgvspt = directories["Rg"]->Get<TH2 >(Form("hRgAbsR%02d", R)),
             thetagvspt = directories["Thetag"]->Get<TH2>(Form("hThetagAbsR%02d", R)),
             nsdvspt = directories["Nsd"]->Get<TH2>(Form("hNsdAbsR02"));
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

void makeCombinedSpectrumSD_mbkthard_standalone(const char *filebase = "AnalysisResults.root"){
    std::map<int, std::set<rdist>> histosKt;
    for(auto minkt : findKtMin(gSystem->GetWorkingDirectory())) {
        std::cout << "Processing min. kt " << minkt << std::endl; 
        histosKt[minkt] = parseFile(Form("%02d/%s", minkt, filebase));
    }
    std::set<rdist> histosMinBias;
    if(!gSystem->AccessPathName("mb", EAccessMode::kFileExists)) {
        histosMinBias = parseFile(Form("mb/%s", filebase));
    }
    std::map<int, int> minktmap_old = {{0,0}, {5., 20.}, {10, 30}, {15, 50.}, {20, 60.}, {30, 80}, {50, 110,}, {90, 150}, {150, 250}, {230, 400}};
    std::map<int, int> minktmap = {{0,0}, {5., 30.}, {10, 40}, {15, 60.}, {20, 70.}, {30, 100}, {40., 120.}, {50, 150,}, {90, 200}, {150, 300}, {230, 500}};
    std::set<rdist> resulthists;

    for(auto &[minkt, histos] : histosKt) {
        auto minpt = minktmap.find(minkt)->second;
        std::cout << "Using data from kt-min for pt > " << minpt << " GeV/c" << std::endl;
        for(auto R : ROOT::TSeqI(2, 7)){
            auto rhistsResult = resulthists.find({R}),
                 rhistsKt = histos.find({R});
            if(rhistsResult == resulthists.end()){
                resulthists.insert(*rhistsKt);    
            } else {
                replace1D(rhistsResult->fSpectrum, rhistsKt->fSpectrum, minpt);
                replace2D(rhistsResult->fZgVsPt, rhistsKt->fZgVsPt, minpt);
                replace2D(rhistsResult->fRgVsPt, rhistsKt->fRgVsPt, minpt);
                replace2D(rhistsResult->fThetagVsPt, rhistsKt->fThetagVsPt, minpt);
                replace2D(rhistsResult->fNsdVsPt, rhistsKt->fNsdVsPt, minpt);
            }
        }
    }
    std::unique_ptr<TFile> writer(TFile::Open("CombinedSD.root", "RECREATE"));
    for(auto bin : resulthists) bin.Write(*writer);
}