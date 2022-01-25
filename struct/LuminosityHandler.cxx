#include "../meta/stl.C"
#include "../meta/root.C"

#include "DataFileHandler.cxx"

class LuminosityHandler {
    public:
        enum class TriggerClass {
            INT7,
            EJ1,
            EJ2
        };

        class YearNotFoundException : public std::exception {
            public:
                YearNotFoundException(int year) : fYear(year), fMessage() {
                    std::stringstream msgbuilder;
                    msgbuilder << "Cross section not found for year " << fYear; 
                    fMessage = msgbuilder.str();
                }
                ~YearNotFoundException() noexcept = default;

                const char *what() const noexcept { return fMessage.data(); }

                int getYear() const { return fYear; }
            private:
                int fYear;
                std::string fMessage;
        };

        LuminosityHandler(const DataFileHandler &handler) : fDataHandler(handler) {}
        ~LuminosityHandler() = default;

        double getLuminosity(TriggerClass trigger, int year, bool correctVTX) {
            double centnotrdcorrection = 1.,
                   averageDownscale = 1.,
                   refXsection = getRefCrossSectionPB(year);
            if(trigger == TriggerClass::EJ2) averageDownscale = getEffectiveDownscaleing(trigger, TriggerCluster::kANY);
            if(trigger == TriggerClass::EJ1) centnotrdcorrection = getCENTNOTRDCorrection();
            double lumi = getRefLuminosity() * averageDownscale * centnotrdcorrection / refXsection;
            if(correctVTX) {
                lumi /= getVertexFindingEfficiency();
            }
            return lumi;
        }

        double getRefLuminosity() const {
            return fDataHandler.getClusterCounterWeighted(2, "INT7").getCounters(TriggerCluster::kANY);
        }

        double getVertexFindingEfficiency() const {
            return fDataHandler.getVertexFindingEfficiency(2, "INT7");
        }

        double getRefCrossSectionPB(int year) const {
            std::map<int, double> crosssections = {{2017, 58.10}, {2018, 57.52}};
            auto xsec = crosssections.find(year);
            if(xsec != crosssections.end()) {
                return xsec->second * 1e9; 
            } 
            throw YearNotFoundException(year);
        }

        double getCENTNOTRDCorrection() const {
            return fDataHandler.getClusterCounterAbs(2, "EJ1").makeClusterRatio(TriggerCluster::kCENTNOTRD, TriggerCluster::kCENT);   
        }

        double getEffectiveDownscaleing(TriggerClass trigger, TriggerCluster clust = TriggerCluster::kANY) {
            double eventsWeighted = fDataHandler.getClusterCounterWeighted(2, getTriggerClassName(trigger)).getCounters(clust),
                   eventsAbs = fDataHandler.getClusterCounterAbs(2, getTriggerClassName(trigger)).getCounters(clust);
            return eventsAbs / eventsWeighted;
        }

    private:
        const char *getTriggerClassName(TriggerClass trigger) {
            switch(trigger) {
                case TriggerClass::INT7: return "INT7";
                case TriggerClass::EJ1: return "EJ1";
                case TriggerClass::EJ2: return "EJ2";
            }
        }

        const DataFileHandler &fDataHandler;
};  