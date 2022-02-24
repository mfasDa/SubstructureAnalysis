#include "../../helpers/string.C"
#include "../../meta/aliphysics.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../meta/stl.C"

enum class LumiDataType_t {
    kLuminosity,
    kEffectiveLuminosity,
    kUncertainty,
    kEffectiveDownscaling
};
    
const std::array<std::string, 3> TRIGGERS = {{"INT7", "EJ1", "EJ2"}};
const std::array<LumiDataType_t, 4> DTYPES = {{LumiDataType_t::kLuminosity, LumiDataType_t::kEffectiveLuminosity, LumiDataType_t::kUncertainty, LumiDataType_t::kEffectiveDownscaling}};

struct LuminosityData {
    std::string mPeriod;
    std::map<std::string, double> mLuminosity;
    std::map<std::string, double> mEffectiveLuminosity;
    std::map<std::string, double> mUncertainty;
    std::map<std::string, double> mEffectiveDownscaling;

    bool operator==(const LuminosityData &other) const { return mPeriod == other.mPeriod; }
    bool operator<(const LuminosityData &other) const { return mPeriod < other.mPeriod; }

    double getValue(const std::string_view trigger, LumiDataType_t dtype) const {
        const std::map<std::string, double> *datamap = nullptr;
        switch(dtype){
        case LumiDataType_t::kLuminosity: datamap = &mLuminosity; break;
        case LumiDataType_t::kEffectiveLuminosity: datamap = &mEffectiveLuminosity; break;
        case LumiDataType_t::kUncertainty: datamap = &mUncertainty; break;
        case LumiDataType_t::kEffectiveDownscaling: datamap = &mEffectiveDownscaling; break;
        };
        auto found = datamap->find(trigger.data());
        if(found != datamap->end()) return found->second;
        return -1.;
    }
};

LuminosityData readFile(const std::string_view filename) {
    std::cout << "Reading " << filename << std::endl;
    auto tokens = tokenize(filename.data(), '/');    
    std::string period;
    auto periodstr  = std::find_if(tokens.begin(), tokens.end(), [](const std::string &tok) { return tok.find("LHC") != std::string::npos; });
    if(periodstr != tokens.end()) period = *periodstr;

    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::string dirnameNorm;
    for(auto obj : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
        std::string_view keyname(obj->GetName());
        if(keyname.find("EmcalTriggerNorm") != std::string::npos) {
            dirnameNorm = keyname;
            break;
        }
    }
    reader->cd(dirnameNorm.data());
    auto lumihists = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    PWG::EMCAL::AliEmcalTriggerLuminosity lumihandler(lumihists);
    lumihandler.Evaluate();
    LuminosityData result;
    result.mPeriod = period;
    const auto LUMIUNIT = PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t::kPb;
    for(const auto &trg : TRIGGERS) {
        result.mLuminosity[trg] = lumihandler.GetLuminosityForTrigger(trg.data(), LUMIUNIT);
        result.mEffectiveLuminosity[trg] = lumihandler.GetEffectiveLuminosityForTrigger(trg.data(), LUMIUNIT);
        result.mUncertainty[trg] = lumihandler.GetLuminosityUncertaintyForTrigger(trg.data());
        result.mEffectiveDownscaling[trg] = lumihandler.GetEffectiveDownscalingForTrigger(trg.data());
    }
    return result;
}

TH1 *makeTrendLumiData(const std::set<LuminosityData> data, const std::string_view trigger, LumiDataType_t dtype) {
    std::string histname, histtitle, yaxis;
    switch(dtype){
    case LumiDataType_t::kLuminosity:
        histname = Form("hLuminosity%s", trigger.data());
        histtitle = Form("Integrated luminosity for trigger %s", trigger.data());
        yaxis = "L_{int} (pb^{-1}";
        break;
    case LumiDataType_t::kEffectiveLuminosity:
        histname = Form("hEffectiveLuminosity%s", trigger.data());
        histtitle = Form("Effective integrated luminosity for trigger %s", trigger.data());
        yaxis = "L_{int} (pb^{-1}";
        break;
    case LumiDataType_t::kUncertainty:
        histname = Form("hUncertainty%s", trigger.data());
        histtitle = Form("Luminosity uncertainty for trigger %s", trigger.data());
        yaxis = "sys. uncertainty";
        break;
    case LumiDataType_t::kEffectiveDownscaling:
        histname = Form("hEffectiveDownscaling%s", trigger.data());
        histtitle = Form("Effective downscaling for trigger %s", trigger.data());
        yaxis = "Downscale factor";
        break;
    };
    auto result = new TH1D(histname.data(), histtitle.data(), data.size(), 0., data.size());
    result->SetDirectory(nullptr);
    result->SetStats(false);
    int currentbin = 1;
    for(const auto &period : data) {
        result->GetXaxis()->SetBinLabel(currentbin, period.mPeriod.data());
        result->SetBinContent(currentbin, period.getValue(trigger, dtype));
        currentbin++;
    }
    return result;
}

ROOT6tools::TSavableCanvas *makePlotDtype(const std::set<LuminosityData> &dataset, LumiDataType_t &dtype) {
    std::string plotname, plottitle;
    switch(dtype){
    case LumiDataType_t::kLuminosity:
        plotname = "TrendingLuminosityPeriods";
        plottitle = "Trending luminosity periods";
        break;
    case LumiDataType_t::kEffectiveLuminosity:
        plotname = "TrendingEffectiveLuminosityPeriods";
        plottitle = "Trending effective luminosity periods";
        break;
    case LumiDataType_t::kUncertainty:
        plotname = "TrendingUncertaintyPeriods";
        plottitle = "Trending luminosity uncertainty periods";
        break;
    case LumiDataType_t::kEffectiveDownscaling:
        plotname = "TrendingDownscalingPeriods";
        plottitle = "Trending effective downscaling periods";
        break;
    };
    ROOT6tools::TSavableCanvas *plot = new ROOT6tools::TSavableCanvas(plotname.data(), plottitle.data(), 1200, 400);
    plot->Divide(3,1);
    int currentpad  = 1;
    for(const auto &trg : TRIGGERS){
        plot->cd(currentpad);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        auto plot = makeTrendLumiData(dataset, trg, dtype);
        plot->Draw();
        currentpad++;
    } 
    plot->cd();
    plot->Update();
    return plot;
}

ROOT6tools::TSavableCanvas *makePlotTrigger(const std::set<LuminosityData> &dataset, const std::string_view trigger) {
    std::string plotname = Form("TrendingLuminosity%s", trigger.data()),
                plottitle = Form("Trending luminosity for %s", trigger.data());
    ROOT6tools::TSavableCanvas *plot = new ROOT6tools::TSavableCanvas(plotname.data(), plottitle.data(), 800, 800);
    plot->Divide(2,2);
    int currentpad  = 1;
    for(const auto &dtype : DTYPES){
        plot->cd(currentpad);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        auto plot = makeTrendLumiData(dataset, trigger, dtype);
        plot->Draw();
        currentpad++;
    } 
    plot->cd();
    plot->Update();
    return plot;

}

std::set<std::string> getListOfPeriods(const std::string_view inputdir) {
    std::set<std::string> result;
    std::string filesstring = gSystem->GetFromPipe("ls -1").Data();     
    for(auto content : tokenize(filesstring, '\n')) {
        if(content.find("LHC") != std::string::npos) {
            std::cout << "Accept period " << content << std::endl;
            if(!gSystem->AccessPathName(Form("%s/AnalysisResults.root", content.data()))) {
                result.insert(content);
            }
        }
    }
    std::cout << "Found " << result.size() << " periods" << std::endl;
    return result;
} 

void makeLuminosityComparisonPeriods(const std::string_view inputdir = "."){
    std::set<LuminosityData> datahandler;
    for(auto period : getListOfPeriods(inputdir)) datahandler.insert(readFile(Form("%s/AnalysisResults.root", period.data())));

    for(const auto &trg : TRIGGERS) {
        auto plot = makePlotTrigger(datahandler, trg);
        plot->SaveCanvas(plot->GetName());
    }

    for(auto dt : DTYPES) {
        auto plot = makePlotDtype(datahandler, dt);
        plot->SaveCanvas(plot->GetName());
    }
}