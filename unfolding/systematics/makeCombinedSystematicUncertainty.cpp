#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/string.C"
#include "../../helpers/root.C"

std::map<std::string, std::string> gIgnore = {{"truncation", "loose"}};

struct Range {
    double fPtMin;
    double fPtMax;
    
    bool operator<(const Range &other) const { return fPtMax <= other.fPtMin; }
    bool operator==(const Range &other) const { return fPtMin == other.fPtMin && fPtMax == other.fPtMax; }
};

class PtBin : public Range {
private:
    TH1 *fDefaultspec;
    TH1 *fVarspec;
    TH1 *fRatio;
    TH1 *fBarlow;

public:
    PtBin() = default;
    PtBin(double ptmin, double ptmax) : Range{ptmin, ptmax}, fDefaultspec(nullptr), fVarspec(nullptr), fRatio(nullptr), fBarlow(nullptr) {}
    PtBin(double ptmin, double ptmax, TH1 *defaultspec, TH1 *varspec, TH1 *ratio, TH1 *barlow) : Range{ptmin, ptmax}, fDefaultspec(defaultspec), fVarspec(varspec), fRatio(ratio), fBarlow(barlow) {}
    virtual ~PtBin() = default;

    void SetDefaultSpec(TH1 *defaultspec) { fDefaultspec = defaultspec; }
    void SetVarSpec(TH1 *varspec) { fVarspec = varspec; }
    void SetRatio(TH1 *ratio) { fRatio = ratio; }
    void SetBarlow(TH1 *barlow) { fBarlow = barlow; }

    TH1 *defaultspec() const { return fDefaultspec; }
    TH1 *varspec() const { return fVarspec; } 
    TH1 *ratio() const { return fRatio; }
    TH1 *barlow() const { return fBarlow; }
};

class sysbin : public Range {
private:
    TH1 *fDefaultspectrum;
    TH1 *fMin;
    TH1 *fMax;

public:
    sysbin() = default;
    sysbin(double ptmin, double ptmax, TH1 *defaultspectrum):
        Range{ptmin, ptmax},
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

class SystematicsDistribution : public Range {
private:
    TH1 *fDefaultdistribution;
    ErrorSource fSum;
    std::vector<ErrorSource> fContributions;
public:
    SystematicsDistribution() = default;
    SystematicsDistribution(double ptmin, double ptmax, TH1 *defaultdist) :
        Range{ptmin, ptmax},
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

    void Draw(TLegend *legend, bool multipanel) const {
        for(auto comp : fContributions) comp.Draw(legend);
        fSum.Draw(legend);
        (new ROOT6tools::TNDCLabel(multipanel ?  0.35 : 0.25, 0.12, multipanel ? 0.75 : 0.55, 0.19, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", fPtMin, fPtMax)))->Draw();
    }

    void Write(TFile &writer) const {
        std::stringstream dirname;
        dirname << "pt_" << int(fPtMin) << "_" << int(fPtMax);
        writer.mkdir(dirname.str().data());
        writer.cd(dirname.str().data());
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

struct VariationResults {
    std::vector<PtBin>  fData;
    PtBin &FindBin(const PtBin &other) { return *std::find(fData.begin(), fData.end(), other); }
};

std::pair<double, double> decodePtTag(std::string pttag) {
    pttag.erase(pttag.find("pt"), 2);
    auto limits = tokenize(pttag, '_');
    return {double(std::stoi(limits[0])), double(std::stoi(limits[1]))};
}

VariationResults readFile(const std::string_view filename){
    VariationResults result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto keys = CollectionToSTL<TKey>(reader->GetListOfKeys());
    auto histfinder = [&keys] (const std::string_view tag, const std::string pttag) -> TH1 * {
        auto res = std::find_if(keys.begin(), keys.end(), [&pttag, &tag] (TKey *k) { return contains(k->GetName(), tag) && contains(k->GetName(), pttag); });
        if(res != keys.end()) {
            return(*res)->ReadObject<TH1>();
        }
        return nullptr;
    };
    for(auto k : keys) {
        if(contains(k->GetName(), "default")) {
            std::string keyname = k->GetName();
            auto pttag = keyname.substr(keyname.find("pt"));
            auto    defaulthist = k->ReadObject<TH1>();
            auto    varhist = histfinder("variation", pttag),
                    ratiohist = histfinder("ratioDefaultVar", pttag),
                    barlowhist = histfinder("barlowtest", pttag);
            defaulthist->SetDirectory(nullptr);
            varhist->SetDirectory(nullptr);
            ratiohist->SetDirectory(nullptr);
            barlowhist->SetDirectory(nullptr);
            auto ptlimits = decodePtTag(pttag);
            result.fData.emplace_back(ptlimits.first, ptlimits.second, defaulthist, varhist, ratiohist, barlowhist);
        }
    }
    return result;
}

std::vector<std::string> gettestfiles(const std::string_view dirname, const std::string_view tag){
    std::string allfiles = gSystem->GetFromPipe(Form("ls -1 %s | grep %s", dirname.data(), tag.data())).Data();
    return tokenize(allfiles);
}

std::vector<VariationResults> getVariations(const std::string_view variation, const std::string_view basedir, const std::string_view tag) {
    std::stringstream vardir;
    vardir << basedir << "/" << variation;
    std::vector<VariationResults> result;
    for(auto &t : gettestfiles(vardir.str(), tag)) {
        bool doignore = false;
        for(auto ignore : gIgnore){
            if(variation == ignore.first && contains(t, ignore.second)) {
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

std::vector<sysbin> evaluateSystematics(std::vector<VariationResults> &variations) {
    std::vector<sysbin> sysbins;
    for(auto &v : variations[0].fData){
        sysbin resultbin(v.fPtMin, v.fPtMax, v.defaultspec());
        resultbin.update(v.varspec());
        if(variations.size() > 1) {
            for(auto w : ROOT::TSeqI(1, variations.size())) {
                auto othervar = variations[w].FindBin(v);
                resultbin.update(othervar.varspec());
            }
        } else {
            resultbin.symmetrize();
        }
        sysbins.push_back(resultbin);
    }
    return sysbins;
}

std::set<SystematicsDistribution> createResult(const std::map<std::string, std::vector<sysbin>> errorsources){
    const std::map<std::string, Color_t> colors = {{"binning", kRed}, {"emcaltimecut", kBlue}, {"priors", kGreen+2}, {"regularization", kOrange+5}, {"seeding", kMagenta+1}, {"trackingeff", kCyan+1}, {"truncation", kViolet+1}};
    std::set<SystematicsDistribution> result;
    for(auto b : errorsources.begin()->second) {
        Range findrange{b.fPtMin, b.fPtMax};
        SystematicsDistribution mysys(b.fPtMin, b.fPtMax, b.defaultspectrum());
        for(auto v : errorsources) {
           auto errorbin = *std::find_if(v.second.begin(), v.second.end(), [&findrange](const sysbin &myb) { return myb == findrange; });
           mysys.AddErrorSource(v.first, errorbin.min(), errorbin.max(), colors.find(v.first)->second);
        }
        result.insert(mysys);
    }
    return result;
}

void makeCombinedSystematicUncertainty(double radius, const std::string_view trigger){
    std::string tag = Form("R%02d_%s", int(radius * 10.), trigger.data()); 
    std::vector<std::string> sources = {"binning", "emcaltimecut", "priors", "regularization", "seeding", "trackingeff", "truncation"};
    std::string basedir = gSystem->GetWorkingDirectory();
    std::map<std::string, std::vector<sysbin>> errors;
    for(auto s : sources) {
        auto variations = getVariations(s, basedir, tag);
        auto sysbins = evaluateSystematics(variations);
        errors[s] = sysbins;
    }

    auto distributions = createResult(errors);

    // Display options:
    // 1st One overview plot
    auto sumplot = new ROOT6tools::TSavableCanvas(Form("systematicsSum_%s", tag.data()), Form("Systematics overview R=%.1f, %s", radius, trigger.data()), 1200, 1000);
    sumplot->DivideSquare(distributions.size());
    int ipad = 1;
    for(auto &d : distributions) {
        sumplot->cd(ipad);
        gPad->SetLeftMargin(0.14);
        gPad->SetRightMargin(0.04);
        (new ROOT6tools::TAxisFrame(Form("sumframe%d_%s", ipad, tag.data()), "z_{g}", "Systematic uncertainty (%)", 0., 0.55, -0.3, 0.3))->Draw("axis");
        if(ipad == 1) (new ROOT6tools::TNDCLabel(0.32, 0.8, 0.75, 0.89, Form("Full jets, R=%.1f, %s", radius, trigger.data())))->Draw();
        TLegend *leg(nullptr);
        if(ipad == 2) {
            leg = new ROOT6tools::TDefaultLegend(0.45, 0.65, 0.92, 0.89);
            leg->SetNColumns(2);
            leg->SetTextSize(0.0435);
            leg->Draw();
        }
        d.Draw(leg, true);
        ipad++;
    }
    sumplot->cd();
    sumplot->Update();
    sumplot->SaveCanvas(sumplot->GetName());

    // 2nd one canvas per pt bin
    for(auto &d : distributions){
        auto ptplot = new ROOT6tools::TSavableCanvas(Form("systematicsBin_%d_%d_%s", int(d.fPtMin), int(d.fPtMax), tag.data()), Form("Systematic uncertainty R=%.1f, %s, pt [%.f, %.2f]", radius, trigger.data(), d.fPtMin, d.fPtMax), 800, 600);
        ptplot->cd();
        (new ROOT6tools::TAxisFrame(Form("ptframe_%d_%d_%s", int(d.fPtMin), int(d.fPtMax), tag.data()), "z_{g}", "Systematic uncertainty (%)", 0., 0.7, -0.3, 0.3))->Draw("axis");
        auto *leg = new ROOT6tools::TDefaultLegend(0.7, 0.6, 0.89, 0.89);
        leg->Draw();
        (new ROOT6tools::TNDCLabel(0.32, 0.8, 0.62, 0.89, Form("Full jets, R=%.1f, %s", radius, trigger.data())))->Draw();
        d.Draw(leg, false);
        ptplot->cd();
        ptplot->Update();
        ptplot->SaveCanvas(ptplot->GetName());
    }

    std::unique_ptr<TFile> outwriter(TFile::Open(Form("SystematicsZg_%s.root", tag.data()), "RECREATE"));
    for(auto &d : distributions) d.Write(*outwriter);
}