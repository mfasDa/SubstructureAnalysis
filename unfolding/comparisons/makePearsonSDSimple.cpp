#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

struct ptbin {
    double ptmin;
    double ptmax;
    TH2 *hist;

    bool operator<(const ptbin &other) const { return ptmin < other.ptmin; }
};

std::vector<ptbin> getPearsonHistograms(TDirectory *inputdir, const char *observable) {
    std::vector<ptbin> result;
    std::string tag = Form("pearson%s", observable);
    for(auto object : TRangeDynCast<TKey>(inputdir->GetListOfKeys())) {
        std::string_view objectname(object->GetName());
        if(objectname.find(tag) == std::string::npos) continue;
        std::cout << "Found pearson " << objectname << std::endl;
        std::string ptstring = objectname.substr(objectname.find("pt")+2).data();
        std::cout << "Found pt string " << ptstring << std::endl;
        int separator = ptstring.find("_");
        double ptmin = double(std::stoi(ptstring.substr(0, separator))),
               ptmax = double(std::stoi(ptstring.substr(separator+1)));
        std::cout << "Found pt from " << ptmin << " to " << ptmax << std::endl;
        auto hist = object->ReadObject<TH2>();
        hist->SetDirectory(nullptr);
        result.push_back({ptmin, ptmax, hist});
    }
    std::sort(result.begin(), result.end(), std::less<ptbin>());
    return result;
}

void makePearsonSDSimple(const char *filename = "UnfoldedSD.root", const char *observable = "Zg", int defaultiteration = 6) {
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    reader->cd(observable);
    auto basedir = gDirectory;

    ROOT6tools::TSavableCanvas *currentplot = nullptr;
    int currentpad = 1, currentcanvas = 0;
    for(auto R : ROOT::TSeqI(2, 7)) {
        currentplot = nullptr;
        currentcanvas = 0;
        currentpad = 1;
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        basedir->cd(rstring.data());
        auto rdir = gDirectory;
        rdir->cd(Form("Iter%d", defaultiteration));
        auto pearsonhists = getPearsonHistograms(gDirectory, observable); 
        for(auto [ptmin, ptmax, pearsonhist] : pearsonhists) {
            if(!currentplot || currentpad == 11) {
                if(currentplot) {
                    currentplot->SaveCanvas(currentplot->GetName());
                    currentcanvas++;
                }
                currentplot = new ROOT6tools::TSavableCanvas(Form("pearson%s_%s_%d", observable, rstring.data(), currentcanvas), Form("Pearson coefficients for %s for %s (%d)", observable, rstring.data(), currentcanvas), 1200, 800);
                currentplot->Divide(4,2);
                currentpad = 1;
            }
            currentplot->cd(currentpad);
            pearsonhist->SetTitle(Form("%s (%s), %.1f (GeV/c) < p_{t} < %.1f GeV./c", observable, rstring.data(), ptmin, ptmax));
            pearsonhist->SetStats(false);
            pearsonhist->SetXTitle("Det. level");
            pearsonhist->SetYTitle("Part. level");
            pearsonhist->Draw("colz");
            currentpad++;
        }
        if(currentplot) currentplot->SaveCanvas(currentplot->GetName());
    }
}