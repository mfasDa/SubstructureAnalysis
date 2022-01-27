#ifndef LUMINOSITYHANDLER_H
#define LUMINOSITYHANDLER_H
#include "../../meta/stl.C"
#include "../../meta/root.C"

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

        LuminosityHandler(const DataFileHandler &handler) : fDataHandler(handler) { initReferenceCrossSection(); }
        ~LuminosityHandler() = default;

        double getLuminosity(TriggerClass trigger, bool correctVTX) const {
            double lumi = 0.;
            if(trigger == TriggerClass::INT7){
                // Min. bias is the reference trigger, scale only its events
                lumi = getRawEvents(trigger); 
            } else {
                double centnotrdcorrection = 1.,
                       averageDownscale = 1.;
                if(trigger == TriggerClass::EJ2) averageDownscale = getEffectiveDownscaleing(trigger, TriggerCluster::kANY);
                if(trigger == TriggerClass::EJ1) centnotrdcorrection = getCENTNOTRDCorrection();
                lumi = getRefLuminosity() * averageDownscale * centnotrdcorrection; 
            }
            lumi /= fReferenceCrosssection;
            if(correctVTX) {
                lumi /= getVertexFindingEfficiency();
            }
            return lumi;
        }

        double getRawEvents(TriggerClass trg) const {
            return fDataHandler.getClusterCounterAbs(2, getTriggerClassName(trg)).getCounters(TriggerCluster::kANY);
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

        double getEffectiveDownscaleing(TriggerClass trigger, TriggerCluster clust = TriggerCluster::kANY) const {
            double eventsWeighted = fDataHandler.getClusterCounterWeighted(2, getTriggerClassName(trigger)).getCounters(clust),
                   eventsAbs = fDataHandler.getClusterCounterAbs(2, getTriggerClassName(trigger)).getCounters(clust);
            return eventsAbs / eventsWeighted;
        }

    private:
        const char *getTriggerClassName(TriggerClass trigger) const {
            switch(trigger) {
                case TriggerClass::INT7: return "INT7";
                case TriggerClass::EJ1: return "EJ1";
                case TriggerClass::EJ2: return "EJ2";
            }
        }

        void initReferenceCrossSection() {
            double averageXsection = 57.2e9;
            std::string errortype, message;
            bool isError = false;
            try {
                auto years = fDataHandler.getDataset(2, "INT7").getYearRange();
                if(years.first == years.second) {
                    auto year = years.first;        
                    fReferenceCrosssection = getRefCrossSectionPB(year);
                    std::cout << "Using reference cross section " << fReferenceCrosssection << " for year " << year << std::endl;
                } else {
                    std::cerr << "Year cannot be uniquely determined from " << years.first << " and " << years.second << ", using average cross secrtion " << averageXsection << std::endl;    
                }
            } catch(DataFileHandler::DataNotFoundException &e) {
                errortype = "DataNotFoundException";
                message = e.what();
                isError = true;
            } catch(DataFileHandler::Dataset::PeriodHandlerNotSetException &e) {
                errortype = "PeriodHandlerNotSetException";
                message = e.what();
                isError = true;
            } catch(UninitializedException &e) {
                errortype = "UninitializedException";
                message = e.what();
                isError = true;
            } catch(EventCounterRun::RunNotFoundException &e) {
                errortype = "RunNotFoundException";
                message = e.what();
                isError = true;
            } catch(PeriodHandler::PeriodNotFoundException& e) {
                errortype = "PeriodNotFoundException";
                message = e.what();
                isError = true;
            } catch(...) {
                errortype = "other";
                isError = true;
            }

            if(isError) {
                std::cerr << "Year cnnot be uniquely handled due to exception " << errortype<< "(" << message << "), using average cross section " << averageXsection << " pb" << std::endl;
                fReferenceCrosssection = averageXsection;
            }
        }

        const DataFileHandler &fDataHandler;
        double fReferenceCrosssection = -1.;
};  
#endif