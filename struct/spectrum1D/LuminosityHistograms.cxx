#ifndef __LUMINOSITYHISTOGRAMS_C__
#define __LUMINOSITYHISTOGRAMS_C__

#include "../../meta/aliphysics.C"
#include "../../meta/root.C"
#include "../../meta/stl.C"
#include "../../helpers/root.C"

class LuminosityHistograms {
public:
    enum class HistoData_t {
        kLuminosity,
        kEffectiveLuminosity,
        kUncertainty,
        kObservedDownscaling
    };  
    LuminosityHistograms() = default;
    ~LuminosityHistograms() = default;

    void addYear(int year, PWG::EMCAL::AliEmcalTriggerLuminosity *data) { mLuminosityHandlers[year] = data; }
    void build() {
        if(!mInitialized) {
            build_triggerclasses();
            build_years();
        }
        mInitialized = true;
    }

    const std::map<int, TH1 *> &getLuminosityHistosForYears()  const { return mLuminosityTriggerClasses; }
    const std::map<std::string, TH1 *> &getLuminosityHistosForTriggerClasses() { return mLuminosityYears; }
    const std::map<int, TH1 *> &getEffectiveLuminosityHistosForYears()  const { return mEffectiveLuminosityTriggerClasses; }
    const std::map<std::string, TH1 *> &getEffectiveLuminosityHistosForTriggerClasses() { return mEffectiveLuminosityYears; }
    const std::map<int, TH1 *> &getUncertaintyHistosForYears()  const { return mUncertaintyTriggerClasses; }
    const std::map<std::string, TH1 *> &getUncertaintyHistosForTriggerClasses() { return mUncertaintyYears; }
    const std::map<int, TH1 *> &getObservedDownscalingHistosForYears()  const { return mEffectiveDownscalingTriggerClasses; }
    const std::map<std::string, TH1 *> &getObservedDownscalingHistosForTriggerClasses() { return mEffectiveDownscalingYears; }
    TH1 *getLuminosityHistAllYears() const { return mLuminosityAllYears; }
    TH1 *getEffectiveLuminosityHistAllYears() const { return mEffectiveLuminosityAllYears; }

private:
    void build_triggerclasses(){
        std::array<std::string, 3> triggerclasses = {{"INT7", "EJ1", "EJ2"}};
        std::map<HistoData_t, std::map<std::string, TH1 *>> container = {
            {HistoData_t::kLuminosity, mLuminosityYears}, 
            {HistoData_t::kEffectiveLuminosity, mEffectiveLuminosityYears}, 
            {HistoData_t::kUncertainty, mUncertaintyYears}, 
            {HistoData_t::kObservedDownscaling, mEffectiveDownscalingYears}
        };
        for(auto &[datatype, cont] : container) {
            for(auto &trg : triggerclasses) {
                auto hist = build_triggerclass(trg, datatype);
                if(hist) cont[trg] = hist; 
            } 
        }
    }

    void build_years() {
        std::map<HistoData_t, std::map<int, TH1 *>> container = {
            {HistoData_t::kLuminosity, mLuminosityTriggerClasses}, 
            {HistoData_t::kEffectiveLuminosity, mEffectiveLuminosityTriggerClasses}, 
            {HistoData_t::kUncertainty, mUncertaintyTriggerClasses}, 
            {HistoData_t::kObservedDownscaling, mEffectiveDownscalingTriggerClasses}
        };
        for(auto &[datatype, cont] : container) {
            for(auto &[year, handler] : mLuminosityHandlers) {
                if(!handler) std::cerr << "LuminosityHistograms -  build_years: Not found handler for year " << year << std::endl;
                auto hist = build_year(year, datatype);
                if(hist) cont[year] = hist; 
            }
        }

        // Make sum all years
        for(auto &[year, lumihist] : mLuminosityTriggerClasses) {
            if(!mLuminosityAllYears) {
                mLuminosityAllYears = histcopy(lumihist);
                mLuminosityAllYears->SetNameTitle("hLuminositiesAllYears", "Integrated luminosities for all years");
                mLuminosityAllYears->SetDirectory(nullptr);
            } else {
                mLuminosityAllYears->Add(lumihist);
            }
        }
        for(auto &[year, lumihist] : mEffectiveLuminosityTriggerClasses) {
            if(!mEffectiveLuminosityAllYears) {
                mEffectiveLuminosityAllYears = histcopy(lumihist);
                mEffectiveLuminosityAllYears->SetNameTitle("hEffectiveLuminositiesAllYears", "Effective integrated luminosities for all years");
                mEffectiveLuminosityAllYears->SetDirectory(nullptr);
            } else {
                mEffectiveLuminosityAllYears->Add(lumihist);
            }
        }
    }

    TH1 *build_triggerclass(const std::string_view trigger, HistoData_t datatype) {
        int currentmin = INT_MAX, currentmax = INT_MIN;
        for(auto &[year, handler] : mLuminosityHandlers) {
            if(year > currentmax) currentmax = year;
            if(year < currentmin) currentmin = year; 
        } 
        double yearmin = currentmin - 0.5,
               yearmax = currentmax + 0.5;
        int nyears = int(yearmax - yearmin);
        std::string histname, histtitle, ytitle;
        switch(datatype) {
        case HistoData_t::kLuminosity: {
            histname = Form("hLuminosities%s", trigger.data());
            histtitle = Form("Integrated luminosities for trigger %s", trigger.data());
            ytitle = "L_{int} (pb^{-1})";
        }
        case HistoData_t::kEffectiveLuminosity: {
            histname = Form("hEffectiveLuminosities%s", trigger.data());
            histtitle = Form("Effective integrated luminosities for trigger %s", trigger.data());
            ytitle = "L_{int} (pb^{-1})";
        }
        case HistoData_t::kUncertainty: {
            histname = Form("hLuminosityUncertainties%s", trigger.data());
            histtitle = Form("Luminosity uncertainties for trigger %s", trigger.data());
            ytitle = "Systematic uncertainty";
        }
        case HistoData_t::kObservedDownscaling: {
            histname = Form("hObservedDownscaling%s", trigger.data());
            histtitle = Form("Observed downscaling for trigger %s", trigger.data());
            ytitle = "Downscale factor";
        }
        };
        auto hist = new TH1F(histname.data(), histtitle.data(), nyears, yearmin, yearmax);
        hist->GetXaxis()->SetTitle("Year");
        hist->GetYaxis()->SetTitle(ytitle.data());
        hist->SetDirectory(nullptr);
        for(auto &[year, handler] : mLuminosityHandlers) {
            if(!handler) std::cerr << "LuminosityHistograms -  build_triggerclass: Not found handler for year " << year << std::endl;
            double lumi = handler->GetLuminosityForTrigger(trigger.data(), PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t::kPb);
            hist->SetBinContent(hist->GetXaxis()->FindBin(year), lumi);
        } 
        return hist;
    }

    TH1 *build_year(int year, HistoData_t datatype) {
        PWG::EMCAL::AliEmcalTriggerLuminosity *handler = nullptr;
        auto found = mLuminosityHandlers.find(year);
        if(found == mLuminosityHandlers.end()) {
            return nullptr;
        }
        handler = found->second;
        std::array<std::string, 3> triggerclasses = {{"INT7", "EJ1", "EJ2"}};
        std::string histname, histtitle, ytitle;
        switch(datatype) {
        case HistoData_t::kLuminosity: {
            histname = Form("hLuminosities%d", year);
            histtitle = Form("Integrated luminosities for %d", year);
            ytitle = "L_{int} (pb^{-1})";
        }
        case HistoData_t::kEffectiveLuminosity: {
            histname = Form("hEffectiveLuminosities%d", year);
            histtitle = Form("Effective integrated luminosities for %d", year);
            ytitle = "L_{int} (pb^{-1})";
        }
        case HistoData_t::kUncertainty: {
            histname = Form("hLuminosityUncertainties%d", year);
            histtitle = Form("Luminosity uncertainties for %d", year);
            ytitle = "Systematic uncertainty";
        }
        case HistoData_t::kObservedDownscaling: {
            histname = Form("hObservedDownscaling%d", year);
            histtitle = Form("Observed downscaling for %d", year);
            ytitle = "Downscale factor";
        }
        };
        auto hist = new TH1F(histname.data(), histtitle.data(), 3, 0, 3);
        hist->SetDirectory(nullptr);
        hist->GetXaxis()->SetTitle("Year");
        hist->GetYaxis()->SetTitle(ytitle.data());
        for(int i = 0; i < 3; i++) hist->GetXaxis()->SetBinLabel(i+1, triggerclasses[i].data());
        for(auto &trg : triggerclasses){
            double lumi = handler->GetLuminosityForTrigger(trg.data(), PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t::kPb);
            hist->SetBinContent(hist->GetXaxis()->FindBin(trg.data()), getData(handler, trg, datatype));
        }
        return hist;
    }

    double getData(PWG::EMCAL::AliEmcalTriggerLuminosity *handler, const std::string_view trigger, HistoData_t datatype) {
        const PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t unit = PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t::kPb;
        double value = 0.;
        switch(datatype) {
        case HistoData_t::kLuminosity: value = handler->GetLuminosityForTrigger(trigger.data(), unit); break;
        case HistoData_t::kEffectiveLuminosity: value = handler->GetEffectiveLuminosityForTrigger(trigger.data(), unit); break;
        case HistoData_t::kUncertainty: value = handler->GetLuminosityUncertaintyForTrigger(trigger.data()); break;
        case HistoData_t::kObservedDownscaling: value = handler->GetEffectiveDownscalingForTrigger(trigger.data()); break;
        };
        return value;
    }

    std::map<int, PWG::EMCAL::AliEmcalTriggerLuminosity *> mLuminosityHandlers;
    std::map<int, TH1 *> mLuminosityTriggerClasses;
    std::map<std::string, TH1 *> mLuminosityYears;
    std::map<int, TH1 *> mEffectiveLuminosityTriggerClasses;
    std::map<std::string, TH1 *> mEffectiveLuminosityYears;
    std::map<int, TH1 *> mUncertaintyTriggerClasses;
    std::map<std::string, TH1 *> mUncertaintyYears;
    std::map<int, TH1 *> mEffectiveDownscalingTriggerClasses;
    std::map<std::string, TH1 *> mEffectiveDownscalingYears;
    TH1 *mLuminosityAllYears = nullptr;
    TH1 *mEffectiveLuminosityAllYears = nullptr;
    bool mInitialized = false;
};

#endif