#ifndef __LUMINOSITYHISTOGRAMS_C__
#define __LUMINOSITYHISTOGRAMS_C__

#include "../../meta/aliphysics.C"
#include "../../meta/root.C"
#include "../../meta/stl.C"
#include "../../helpers/root.C"

class LuminosityHistograms {
public:
    LuminosityHistograms() = default;
    ~LuminosityHistograms() = default;

    void addYear(int year, PWG::EMCAL::AliEmcalTriggerLuminosity *data) { mLuminosityHandlers[year] = data; }
    void build() {
        build_triggerclasses();
        build_years();
    }

    const std::map<int, TH1 *> &getLuminosityHistosForYears()  const { return mLuminosityTriggerClasses; }
    const std::map<std::string, TH1 *> &getLuminosityHistosForTriggerClasses() { return mLuminosityYears; }
    TH1 *getLuminosityHistAllYears() const { return mLuminosityAllYears; }

private:
    void build_triggerclasses(){
        std::array<std::string, 3> triggerclasses = {{"INT7", "EJ1", "EJ2"}};
        for(auto &trg : triggerclasses) {
            auto hist = build_triggerclass(trg);
            if(hist) mLuminosityYears[trg] = hist; 
        } 
    }

    void build_years() {
        for(auto &[year, handler] : mLuminosityHandlers) {
            auto hist = build_year(year);
            if(hist) mLuminosityTriggerClasses[year] = hist; 
        }

        // Make sum all years
        for(auto &[year, lumihist] : mLuminosityTriggerClasses) {
            if(!mLuminosityAllYears) {
                mLuminosityAllYears = histcopy(lumihist);
                mLuminosityAllYears->SetNameTitle("hLuminositiesAllYears", "Luminosities for all years");
                mLuminosityAllYears->SetDirectory(nullptr);
            } else {
                mLuminosityAllYears->Add(lumihist);
            }
        }
    }

    TH1 *build_triggerclass(const std::string_view trigger) {
        int currentmin = -1, currentmax = INT_MAX;
        for(auto &[year, handler] : mLuminosityHandlers) {
            if(year > currentmax) currentmax = year;
            if(year < currentmin) currentmin = year; 
        } 
        double yearmin = currentmin - 0.5,
               yearmax = currentmax + 0.5;
        int nyears = int(yearmax - yearmin);
        auto hist = new TH1F(Form("hLuminosities%s", trigger.data()), Form("Integrated Luminosities for trigger %s", trigger.data()), nyears, yearmin, yearmax);
        hist->GetXaxis()->SetTitle("Year");
        hist->GetYaxis()->SetTitle("L_{int} (pb^{-1})");
        hist->SetDirectory(nullptr);
        for(auto &[year, handler] : mLuminosityHandlers) {
            double lumi = handler->GetLuminosityForTrigger(trigger.data(), PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t::kPb);
            hist->SetBinContent(hist->GetXaxis()->FindBin(year), lumi);
        } 
        return hist;
    }

    TH1 *build_year(int year) {
        PWG::EMCAL::AliEmcalTriggerLuminosity *handler = nullptr;
        auto found = mLuminosityHandlers.find(year);
        if(found == mLuminosityHandlers.end()) {
            return nullptr;
        }
        std::array<std::string, 3> triggerclasses = {{"INT7", "EJ1", "EJ2"}};
        auto hist = new TH1F(Form("hLuminosities%d", year), Form("Integrated Luminosities for %d", year), 3, 0, 3);
        hist->SetDirectory(nullptr);
        hist->GetXaxis()->SetTitle("Year");
        hist->GetYaxis()->SetTitle("L_{int} (pb^{-1})");
        for(int i = 0; i < 3; i++) hist->GetXaxis()->SetBinLabel(i+1, triggerclasses[i].data());
        for(auto &trg : triggerclasses){
            double lumi = handler->GetLuminosityForTrigger(trg.data(), PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t::kPb);
            hist->SetBinContent(hist->GetXaxis()->FindBin(trg.data()), lumi);
        }
        return hist;
    }

    std::map<int, PWG::EMCAL::AliEmcalTriggerLuminosity *> mLuminosityHandlers;
    std::map<int, TH1 *> mLuminosityTriggerClasses;
    std::map<std::string, TH1 *> mLuminosityYears;
    TH1 *mLuminosityAllYears = nullptr;
};

#endif