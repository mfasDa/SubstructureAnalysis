
#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/string.C"
#include "../../helpers/root.C"

std::map<std::string, std::string> gIgnore = {{"truncation", "loose"}};

class Variation {
private:
    TH1 *fDefaultspec;
    TH1 *fVarspec;
    TH1 *fRatio;
    TH1 *fBarlow;

public:
    Variation() = default;
    Variation(TH1 *defaultspec, TH1 *varspec, TH1 *ratio, TH1 *barlow) : fDefaultspec(defaultspec), fVarspec(varspec), fRatio(ratio), fBarlow(barlow) {}
    virtual ~Variation() = default;

    void SetDefaultSpec(TH1 *defaultspec) { fDefaultspec = defaultspec; }
    void SetVarSpec(TH1 *varspec) { fVarspec = varspec; }
    void SetRatio(TH1 *ratio) { fRatio = ratio; }
    void SetBarlow(TH1 *barlow) { fBarlow = barlow; }

    TH1 *defaultspec() const { return fDefaultspec; }
    TH1 *varspec() const { return fVarspec; } 
    TH1 *ratio() const { return fRatio; }
    TH1 *barlow() const { return fBarlow; }
};

class sysbin {
private:
    TH1 *fDefaultspectrum;
    TH1 *fMin;
    TH1 *fMax;

public:
    sysbin() = default;
    sysbin(TH1 *defaultspectrum):
        fDefaultspectrum(defaultspectrum),
        fMin(nullptr),
        fMax(nullptr)
    {
        fMin = histcopy(defaultspectrum);
        fMin->SetDirectory(nullptr);
        fMin->Reset();
        fMax = histcopy(defaultspectrum);
        fMax->SetDirectory(nullptr);
        fMax->Reset();
    }
    virtual ~sysbin() = default;

    void update(TH1 * varspectrum) {
        for(auto b : ROOT::TSeqI(0, fDefaultspectrum->GetXaxis()->GetNbins())){
            auto diff = (varspectrum->GetBinContent(b+1) - fDefaultspectrum->GetBinContent(b+1)) / fDefaultspectrum->GetBinContent(b+1);
            if(diff > fMax->GetBinContent(b+1)) fMax->SetBinContent(b+1, diff);
            if(diff < fMin->GetBinContent(b+1)) fMin->SetBinContent(b+1, diff);
        }
    }

    void symmetrize() {
        for(auto b : ROOT::TSeqI(0, fMax->GetNbinsX())){
            auto minv = fMin->GetBinContent(b+1), maxv = fMax->GetBinContent(b+1);
            auto largest = TMath::Max(TMath::Abs(minv), TMath::Abs(maxv));
            fMin->SetBinContent(b+1, -1. * largest);
            fMax->SetBinContent(b+1, largest);
        }
    }

    TH1 *defaultspectrum() const { return fDefaultspectrum; }
    TH1 *min() const { return fMin; }
    TH1 *max() const { return fMax; }
};

struct ErrorSource {
    std::string fName;
    TH1 *fMin;
    TH1 *fMax;
    Color_t fColor;

    bool operator==(const ErrorSource &other) const { return fName == other.fName; }
    bool operator<(const ErrorSource &other) const { return fName < other.fName; }

    void Draw(TLegend *legend) const {
        bool updateLegend = legend ? true : false;
        for(auto b : ROOT::TSeqI(0, fMin->GetNbinsX())){
            if(fMin->GetXaxis()->GetBinUpEdge(b+1) > 250.) break;
            auto box = new TBox(fMin->GetXaxis()->GetBinLowEdge(b+1), TMath::Max(-0.3, fMin->GetBinContent(b+1)), fMin->GetXaxis()->GetBinUpEdge(b+1), TMath::Min(fMax->GetBinContent(b+1), 0.3));
            box->SetLineColor(fColor);
            box->SetFillStyle(0);
            box->Draw();
            if(fMin->GetBinContent(b+1) < -0.3) {
                auto minarrow = new TArrow(fMin->GetXaxis()->GetBinCenter(b+1), -0.25, fMin->GetXaxis()->GetBinCenter(b+1), -0.29);
                minarrow->SetArrowSize(0.02);
                minarrow->Draw();
            }
            if(fMax->GetBinContent(b+1) > 0.3) {
                auto maxarrow = new TArrow(fMax->GetXaxis()->GetBinCenter(b+1), 0.25, fMax->GetXaxis()->GetBinCenter(b+1), 0.29);
                maxarrow->SetArrowSize(0.02);
                maxarrow->Draw();
            }
            if(updateLegend) {
                if(legend) legend->AddEntry(box, fName.data(), "l");
                updateLegend = false;         
            }
        }
    }

    void Write(TDirectory *basedir) const {
        basedir->mkdir(fName.data());
        basedir->cd(fName.data());
        fMin->Write("lowsysrel");
        fMax->Write("upsysrel");
    }
};

class SystematicsDistribution {
private:
    TH1 *fDefaultdistribution;
    ErrorSource fSum;
    std::vector<ErrorSource> fContributions;
public:
    SystematicsDistribution() = default;
    SystematicsDistribution(TH1 *defaultdist) :
        fDefaultdistribution(defaultdist),
        fSum({"sum", nullptr, nullptr, kBlack})
    {   

    }
    virtual ~SystematicsDistribution() = default; 


    void AddErrorSource(const std::string_view name, TH1 *min, TH1 *max, Color_t color) {
        ErrorSource source{std::string(name), min, max, color};
        fContributions.push_back(source);
        UpdateSum(source);
    }

    void Draw(TLegend *legend) const {
        for(auto comp : fContributions) comp.Draw(legend);
        fSum.Draw(legend);
    }

    void Write(TFile &writer) const {
        fDefaultdistribution->Write("reference");
        auto currentdir = gDirectory;
        fSum.Write(currentdir);
        for(auto comp : fContributions) comp.Write(currentdir);
    }

    void UpdateSum(const ErrorSource &source){
        if(!(fSum.fMin && fSum.fMax)) {
            fSum.fMin = source.fMin;
            fSum.fMax = source.fMax;
        } else {
            for(auto b : ROOT::TSeqI(0, fSum.fMin->GetNbinsX())){
                auto lowerval = TMath::Sqrt(TMath::Power(fSum.fMin->GetBinContent(b+1), 2) + TMath::Power(source.fMin->GetBinContent(b+1), 2)),
                     upperval = TMath::Sqrt(TMath::Power(fSum.fMax->GetBinContent(b+1), 2) + TMath::Power(source.fMax->GetBinContent(b+1), 2));
                fSum.fMin->SetBinContent(b+1, -1 * lowerval);
                fSum.fMax->SetBinContent(b+1, upperval);
            }
        }
    }
};

Variation readFile(const std::string_view filename){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto defspectrum = static_cast<TH1 *>(reader->Get("DefaultRatio")),
         varspectrum = static_cast<TH1 *>(reader->Get("VariationRatio")),
         ratiospectra = static_cast<TH1 *>(reader->Get("RatioRatios")),
         barlowtest = static_cast<TH1 *>(reader->Get("barlowtest"));
    defspectrum->SetDirectory(nullptr);
    varspectrum->SetDirectory(nullptr);
    ratiospectra->SetDirectory(nullptr);
    barlowtest->SetDirectory(nullptr);
    return {defspectrum, varspectrum, ratiospectra, barlowtest};
}

std::vector<std::string> gettestfiles(const std::string_view dirname, const std::string_view tag){
    std::string allfiles = gSystem->GetFromPipe(Form("ls -1 %s | grep systematics | grep %s", dirname.data(), tag.data())).Data();
    return tokenize(allfiles);
}

std::vector<Variation> getVariations(const std::string_view variation, const std::string_view basedir, const std::string_view tag) {
    std::stringstream vardir;
    vardir << basedir << "/" << variation;
    std::vector<Variation> result;
    for(auto &t : gettestfiles(vardir.str(), tag)) {
        bool doignore = false;
        for(auto ignore : gIgnore){
            if(static_cast<std::string>(variation) == ignore.first && contains(t, ignore.second)) {
                doignore = true;
                break;
            } 
        }
        if(doignore) {
            std::cout << "Ignoring systematics " << t << std::endl;
            continue;
        }
        std::stringstream testfilename;
        testfilename << vardir.str() << "/" << t;
        result.push_back(readFile(testfilename.str().data()));   
    }
    return result;
}

sysbin evaluateSystematics(std::vector<Variation> &variations) {
    sysbin result(variations[0].defaultspec());
    result.update(variations[0].varspec());
    if(variations.size() > 1) {
        for(auto w : ROOT::TSeqI(1, variations.size())) {
            result.update(variations[w].varspec());
        }
    } else {
        result.symmetrize();
    }
    return result;
}

SystematicsDistribution createResult(const std::map<std::string, sysbin> errorsources){
    const std::map<std::string, Color_t> colors = {{"binning", kRed}, {"clusterizerAlgorithm", kTeal+ 3}, {"emcaltimecut", kBlue}, 
                                                   {"hadronicCorrection", kGray+2}, {"priors", kGreen+2}, {"regularization", kOrange+5}, 
                                                   {"seeding", kMagenta+1}, {"trackingeff", kCyan+1}, {"triggereff", kRed -4},
                                                   {"truncation", kViolet+1}, {"unfoldingmethod", kBlue +9}};
    SystematicsDistribution result(errorsources.begin()->second.defaultspectrum());
    for(auto b : errorsources) result.AddErrorSource(b.first, b.second.min(), b.second.max(), colors.find(b.first)->second);
    return result;
}

void makeCombinedSystematicUncertaintyRatio1D(double radiusnum, double radiusden){
    std::string tag = Form("R%02dR%02d", int(radiusnum * 10.), int(radiusden *10.));  
    //std::vector<std::string> sources = {"binning", "clusterizerAlgorithm", "hadronicCorrection", "emcaltimecut", "priors", "regularization", 
    //                                    "seeding", "trackingeff", "triggereff", "truncation", "unfoldingmethod"};
    std::vector<std::string> sources = {"binning", "clusterizerAlgorithm", "hadronicCorrection", "emcaltimecut", "regularization", 
                                        "seeding", "trackingeff", "triggereff", "truncation", "unfoldingmethod"};
    std::string basedir = gSystem->GetWorkingDirectory();
    std::map<std::string, sysbin> errors;
    for(auto s : sources) {
        auto variations = getVariations(s, basedir, tag);
        auto sysbins = evaluateSystematics(variations);
        errors[s] = sysbins;
    }
    auto sysresult = createResult(errors);

    auto plot = new ROOT6tools::TSavableCanvas(Form("systematics1DPt_%s", tag.data()), Form("Systematic uncertainty R=%.1f/R=%.1f", radiusnum, radiusden), 1000, 600);
    plot->cd();
    gPad->SetRightMargin(0.35);
    (new ROOT6tools::TAxisFrame("sysframe", "p_{t} (GeV/c)", "Systematic uncertainty (%)", 0., 250., -0.3, 0.5))->Draw("axis");
    auto *leg = new ROOT6tools::TDefaultLegend(0.7, 0.3, 0.89, 0.89);
    leg->Draw();
    (new ROOT6tools::TNDCLabel(0.32, 0.15, 0.62, 0.3, Form("Full jets, R=%.1f/R=%.1f", radiusnum, radiusden)))->Draw();
    sysresult.Draw(leg);
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());

    std::unique_ptr<TFile> outwriter(TFile::Open(Form("Systematics1DPt_%s.root", tag.data()), "RECREATE"));
    sysresult.Write(*outwriter);
}