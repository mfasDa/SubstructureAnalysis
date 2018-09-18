#include "../../meta/root.C"
#include "../../meta/root6tools.C"

const std::map<std::string, Color_t> kColorMap = {{"binning", kRed}, {"emcaltimecut", kBlue}, {"priors", kGreen+2}, {"regularization", kOrange+5}, 
                                                   {"seeding", kMagenta+1}, {"trackingeff", kCyan+1}, {"truncation", kViolet+1}, {"clusterizer", kTeal+ 3}, 
                                                   {"hadCorr", kGray+2}, {"triggerresponse", kRed-6}, {"sum", kBlack}};
const std::map<std::string, std::string> kTitleMap = {{"binning", "Binning"}, {"emcaltimecut", "EMCAL time cut"}, {"priors", "Priors"}, {"regularization", "Regularization"}, 
                                                   {"seeding", "EMCAL seeding"}, {"trackingeff", "Tracking efficiency"}, {"truncation", "Truncation"}, {"clusterizer", "EMCAL clusterizer"}, 
                                                   {"hadCorr", "EMCAL cluster hadronic correction"}, {"triggerresponse", "Trigger response"}, {"sum", "Sum"}};

struct ErrorSource {
    std::string fName;
    TH1 *fMin;
    TH1 *fMax;

    void Draw(TLegend *legend) const {
        bool updateLegend = legend ? true : false;
        for(auto b : ROOT::TSeqI(0, fMin->GetNbinsX())){
            auto box = new TBox(fMin->GetXaxis()->GetBinLowEdge(b+1), TMath::Max(-0.3, fMin->GetBinContent(b+1)), fMin->GetXaxis()->GetBinUpEdge(b+1), TMath::Min(fMax->GetBinContent(b+1), 0.3));
            box->SetLineColor(kColorMap.find(fName)->second);
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
                if(legend) legend->AddEntry(box, kTitleMap.find(fName)->second.data(), "l");
                updateLegend = false;         
            }
        }
    }
};

class SystematicZgDist {
    double fPtmin;
    double fPtmax;
    TH1 *fReference;
    ErrorSource fSum;
    std::vector<ErrorSource> fSources;

public:
    SystematicZgDist() = default;
    SystematicZgDist(double ptmin, double ptmax, TH1 *refspec) :
        fPtmin(ptmin),
        fPtmax(ptmax),
        fReference(refspec),
        fSum(),
        fSources()
    {
    }
    virtual ~SystematicZgDist() = default;

    bool operator==(const SystematicZgDist &other) const { return TMath::Abs(fPtmin - other.fPtmin) < DBL_EPSILON && TMath::Abs(fPtmax - other.fPtmax) < DBL_EPSILON; }
    bool operator<(const SystematicZgDist &other) const { return fPtmax <= other.fPtmin; }

    void SetCombinedErrorSource(ErrorSource &source) { fSum = source; }
    void AddErrorSource(const ErrorSource &source) { fSources.push_back(source); }
    
    double ptmin() const { return fPtmin; }
    double ptmax() const { return fPtmax; }

    void Draw(TLegend *legend) const {
        for(auto comp : fSources) comp.Draw(legend);
        fSum.Draw(legend);
        (new ROOT6tools::TNDCLabel(0.35, 0.12, 0.75, 0.19, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", fPtmin, fPtmax)))->Draw();
    }
};

std::pair<double, double> decodePtBin(const std::string_view dirname){
	auto ptstring = dirname.substr(dirname.find_first_of("_") + 1);
	auto delimiter = ptstring.find_first_of("_");
	return { double(std::stoi(static_cast<std::string>(ptstring.substr(0, delimiter)))), double(std::stoi(static_cast<std::string>(ptstring.substr(delimiter+1)))) };
}

std::set<SystematicZgDist> readFile(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::set<SystematicZgDist> result;

    for(auto b : TRangeDynCast<TKey>(reader->GetListOfKeys())){
        auto limits = decodePtBin(b->GetName());
        reader->cd(b->GetName());

        TH1 *refspectrum = static_cast<TH1 *>(gDirectory->Get("reference"));
        refspectrum->SetDirectory(nullptr);
        SystematicZgDist syspt{limits.first, limits.second, refspectrum};

        for(auto s : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
            if(!s->ReadObj()->InheritsFrom("TDirectory")) continue;
            auto basedir = gDirectory;
            gDirectory->cd(s->GetName());
            auto minhist = static_cast<TH1 *>(gDirectory->Get("lowsysrel")),
                 maxhist = static_cast<TH1 *>(gDirectory->Get("upsysrel"));
            minhist->SetDirectory(nullptr);
            maxhist->SetDirectory(nullptr);
            ErrorSource source{s->GetName(), minhist, maxhist};
            if(std::string_view(s->GetName()) == "sum") {
                syspt.SetCombinedErrorSource(source);
            } else {
                syspt.AddErrorSource(source);
            }
            gDirectory = basedir;
        }

        result.insert(syspt);
    }

    return result;
}

void plotSystematicsMultipanel(double radius){
    std::map<std::string, std::set<SystematicZgDist>> triggersystematics;
    std::vector<std::string> triggers = {"INT7", "EJ2", "EJ1"};
    std::map<std::string, std::pair<double, double>> ptranges = {{"INT7", {30., 80.}}, {"EJ2", {80., 120.}}, {"EJ1", {120., 200.}}};
    for(const auto &t : triggers){
        std::stringstream filename;
        filename << "SystematicsZg_R" << std::setw(2) << std::setfill('0') << int(radius*10.) << "_" << t << ".root";
        triggersystematics[t] = readFile(filename.str());
    }

    auto resultplot = new ROOT6tools::TSavableCanvas(Form("combinedSystematicsR%02d", int(radius*10.)), Form("Combined systematic uncertainties R=%.1f", radius), 1400, 1000);
    resultplot->Divide(4,3);
    TLegend *leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.89, 0.89); // For the last panel
    
    int ipad = 1;
    for(const auto &t : triggers){
        auto &ptrange = ptranges.find(t)->second;
        for(const auto &ptbin : triggersystematics.find(t)->second){
            if(ptbin.ptmax() < ptrange.first + 0.5) continue;
            if(ptbin.ptmin() > ptrange.second - 0.5) break;
            resultplot->cd(ipad++);
            gPad->SetLeftMargin(0.15);
            gPad->SetRightMargin(0.05);
            (new ROOT6tools::TAxisFrame(Form("sumframept%d_%d", int(ptbin.ptmin()), int(ptbin.ptmax())), "z_{g}", "Systematic uncertainty (%)", 0., 0.55, -0.3, 0.3))->Draw("axis");
            if(ipad == 2) (new ROOT6tools::TNDCLabel(0.7, 0.8, 0.89, 0.89, Form("Jets, R=%.1f", radius)))->Draw();
            ptbin.Draw(ipad == 2 ? leg : nullptr);
        }
    }
    resultplot->cd(ipad);
    leg->Draw();
    resultplot->cd();
    resultplot->Update();
    resultplot->SaveCanvas(resultplot->GetName());
}