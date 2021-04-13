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

    void Write(TFile &output) const {
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

void makeConvertedSpectrumSD_minbias(const char *inputfile = "AnalysisResults.root"){
    auto rbins = parseFile(inputfile);
    std::unique_ptr<TFile> outputfile(TFile::Open("CombinedSD.root", "RECREATE"));
    for(auto &bn : rbins) bn.Write(*outputfile);
}