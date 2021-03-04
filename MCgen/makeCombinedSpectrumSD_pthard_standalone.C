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

    rdist &operator+=(const rdist &other) {
        fSpectrum->Add(other.fSpectrum);
        fZgVsPt->Add(other.fZgVsPt);
        fRgVsPt->Add(other.fRgVsPt);
        fThetagVsPt->Add(other.fThetagVsPt);
        fNsdVsPt->Add(other.fNsdVsPt);
        return *this;
    }

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

std::set<int> findPtHardBins(const std::string_view inputdir) {
    std::set<int> pthardbins;
    std::unique_ptr<TObjArray> dircontent(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Tokenize("\n"));
    for(auto tok : TRangeDynCast<TObjString>(dircontent.get())) {
        if(tok->String().Contains("bin")) {
            pthardbins.insert(tok->String().ReplaceAll("bin", "").Atoi());
        }
    }
    return pthardbins;
}

void makeCombinedSpectrumSD_pthard_standalone(const char *filename = "AnalysisResults.root") {
    std::map<int, std::set<rdist>> histosPtHard;
    for(auto pthardbin : findPtHardBins(gSystem->GetWorkingDirectory())) {
        std::cout << "Processing pt-hard bin " << pthardbin << std::endl; 
        histosPtHard[pthardbin] = parseFile(Form("bin%d/%s", pthardbin, filename));
    }
    std::vector<rdist> resulthists;
    for(const auto &[pthardbin, histos] : histosPtHard) {
        for(const auto &rhists : histos) {
            auto found = std::find(resulthists.begin(), resulthists.end(), rdist{rhists.fR});
            if(found != resulthists.end()) {
                *found += rhists;
            } else {
                resulthists.push_back(rhists);
            }
        }
    }
    std::unique_ptr<TFile> writer(TFile::Open("CombinedSD.root", "RECREATE"));
    for(auto bin : resulthists) bin.Write(*writer);
}