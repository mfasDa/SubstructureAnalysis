#ifndef DATAFILEHANDLER_H
#define DATAFILEHANDLER_H

#include "../../meta/aliphysics.C"
#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/root.C"
#include "../PeriodHandler.cxx"
#include "../HistogramDataHandler.cxx"
#include "TriggerDefinition.cxx"
#include "RawDataSpectrum.cxx"


class ClusterCounter : public TriggerClusterBinned, public UnmanagedHistogramData<const TH1> {
    public:
        ClusterCounter() = default;
        ClusterCounter(const TH1 *hist) : UnmanagedHistogramData<const TH1>(hist) {}
        ~ClusterCounter() = default;

        double getCounters(TriggerCluster clust) const {
            return getHistogram()->GetBinContent(getClusterBin(clust));
        }

        double makeClusterRatio(TriggerCluster clusterNum, TriggerCluster clusterDen) const {
            return getCounters(clusterNum) / getCounters(clusterDen);
        }
};

class EventCounterRun : public ManagedHistogramData<TH1> {
public:
    class RunNotFoundException : public std::exception {
        public:
            RunNotFoundException(int runnumber) : fRun(runnumber), fMessage() {
                std::stringstream msgbuilder;
                msgbuilder << "Run " << fRun << " not found";
                fMessage = msgbuilder.str();
            }
            ~RunNotFoundException() noexcept = default;

            const char *what() const noexcept { return fMessage.data(); }
            int getRun() const noexcept { return fRun; }

        private:
            int fRun;
            std::string fMessage;
    };
    struct RunRange {
        int fFirstRun;
        int fLastRun;
    };
    EventCounterRun() = default;
    EventCounterRun(const TH1 *datahist) : ManagedHistogramData<TH1>(datahist) { buildRunIndices(); }
    ~EventCounterRun() = default;

    double getEventCountsForRun(int run) const {
        auto data = getHistogram();
        int binID = data->GetXaxis()->FindBin(run);
        if(binID < 1 || binID > data->GetXaxis()->GetNbins()) throw RunNotFoundException(run);
        return data->GetBinContent(binID);
    }

    RunRange getRunRange() const { return {*(fRunNumbers.begin()), *(fRunNumbers.rbegin())}; }

private:
    void buildRunIndices() {
        fRunNumbers.clear();
        auto data = getHistogram();
        for(auto ib: ROOT::TSeqI(0, data->GetXaxis()->GetNbins())) {
            if(data->GetBinContent(ib+1)) {
                fRunNumbers.insert(static_cast<int>(data->GetXaxis()->GetBinLowEdge(ib+1)));
            }
        }
    }
    std::set<int> fRunNumbers;
};

class VertexFindingEfficiency : public ManagedHistogramData<TH1> {
    public:
        VertexFindingEfficiency() = default;
        VertexFindingEfficiency(const TH1 *normalizationHist): ManagedHistogramData<TH1>(normalizationHist) {}
        ~VertexFindingEfficiency() = default;

        operator double() const { return evaluate(); }

        double evaluate() const {
            auto normalizationHist = getHistogram();
            return normalizationHist->GetBinContent(normalizationHist->GetXaxis()->FindBin("Vertex reconstruction and quality")) / normalizationHist->GetBinContent(normalizationHist->GetXaxis()->FindBin("Event selection"));
        }
};


class DataFileHandler {
    public:
        class Dataset {
            public:
                class PeriodHandlerNotSetException : public std::exception {
                public:
                    PeriodHandlerNotSetException() = default;
                    ~PeriodHandlerNotSetException() noexcept = default;
                    const char *what() const noexcept { return "Period handler was not set - cannot determine year or perod(s)"; }
                };
                Dataset() = default;
                ~Dataset() = default;
                Dataset(const TH1 *normalizationHist, const TH1 *runcounter, const TH1 *counterAbs, const TH1 *counterWeighted, const TH2 *spectrumAbs, const TH2 *spectrumWeighted) :
                    fEfficiencyVtx(normalizationHist),
                    fRunCounter(runcounter),
                    fClusterCounterAbs(counterAbs),
                    fClusterCounterWeighted(counterWeighted),
                    fSpectrumAbs(spectrumAbs),
                    fSpectrumWeighted(spectrumWeighted),
                    fPeriodHandler(nullptr)
                {
                }

                const VertexFindingEfficiency &getVertexFindingEfficiency() const { return fEfficiencyVtx; }
                const EventCounterRun &getRunCounter() const { return fRunCounter; }
                const ClusterCounter &getAbsCounters() const { return fClusterCounterAbs; }
                const ClusterCounter &getWeightedCounters() const { return fClusterCounterWeighted; }
                const RawDataSpectrum &getRawSpectrumAbs() const { return fSpectrumAbs; }
                const RawDataSpectrum &getRawSpectrumWeighted() const { return fSpectrumWeighted; }

                std::pair<int, int> getYearRange() const {
                    if(!fPeriodHandler) throw PeriodHandlerNotSetException();
                    auto ranges = fRunCounter.getRunRange();
                    return {fPeriodHandler->getYearForRun(ranges.fFirstRun), fPeriodHandler->getYearForRun(ranges.fLastRun)};
                }

                void setNormalizationHistogram(const TH1 *normalizationHist) { fEfficiencyVtx.setHistogram(normalizationHist); }
                void SetRunCounter(const TH1 *runcounter) { fRunCounter.setHistogram(runcounter); }
                void setAbsCounters(const TH1 *abscounters) { fClusterCounterAbs.setHistogram(abscounters); }
                void setWeightedCounters(const TH1 *weightedcounters) { fClusterCounterWeighted.setHistogram(weightedcounters); }
                void setAbsSpectrum(const TH2 *absspectrum) { fSpectrumAbs.setHistogram(absspectrum); }
                void setWeightedSpectrum(const TH2 *weightedspectrum) { fSpectrumWeighted.setHistogram(weightedspectrum); }
                void setPeriodHandler(PeriodHandler *handler) { fPeriodHandler = handler; }

            private:
                VertexFindingEfficiency fEfficiencyVtx;
                EventCounterRun fRunCounter;
                ClusterCounter fClusterCounterAbs;
                ClusterCounter fClusterCounterWeighted;
                RawDataSpectrum fSpectrumAbs;
                RawDataSpectrum fSpectrumWeighted;
                PeriodHandler *fPeriodHandler;
        };

        class DataNotFoundException : public std::exception {
        public:
            DataNotFoundException() = default;
            ~DataNotFoundException() noexcept = default;

            const char *what() const noexcept { return "Dataset is not found"; }
        };

        DataFileHandler(const std::string_view filename, const std::string_view jettype, const std::string_view sysvar) : 
            fReader(std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"))),
            fJetType(jettype),
            fSysVar(sysvar)
        {
            buildDatasets();
            buildLuminosityHandler();
        }
        ~DataFileHandler() = default;

        const Dataset &getDataset(int R, const std::string_view trigger) const {
           auto found = fDatasets.find(Key{R, trigger});
           if(found == fDatasets.end())  throw DataNotFoundException();
           return found->second;
        }

        const VertexFindingEfficiency &getVertexFindingEfficiency(int R, const std::string_view trigger) const { return  getDataset(R, trigger).getVertexFindingEfficiency(); }
        const ClusterCounter &getClusterCounterAbs(int R, const std::string_view trigger) const { return  getDataset(R, trigger).getAbsCounters(); }
        const ClusterCounter &getClusterCounterWeighted(int R, const std::string_view trigger) const { return  getDataset(R, trigger).getWeightedCounters(); }
        const RawDataSpectrum &getSpectrumAbs(int R, const std::string_view trigger) const { return  getDataset(R, trigger).getRawSpectrumAbs(); }
        const RawDataSpectrum &getSpectrumWeighted(int R, const std::string_view trigger) const { return  getDataset(R, trigger).getRawSpectrumWeighted(); }
        PWG::EMCAL::AliEmcalTriggerLuminosity *getLuminosityHandler() const { if(fLuminosityHandler) return fLuminosityHandler.get(); return nullptr; }

    private:
        class Key {
            public:
                Key() = default;
                Key(int R, std::string_view trigger) : fR(R), fTrigger(trigger) {}
                ~Key() = default;

                bool operator==(const Key &other) const { return fR == other.fR && fTrigger == other.fTrigger; }
                bool operator<(const Key &other) const { 
                    if(fR == other.fR) return fTrigger < other.fTrigger;
                    else return fR < other.fR;
                }
            private:
                int fR;
                std::string fTrigger;
        };

        template<typename T>
        T *HistGetter(TList *list, const char *name) {
            return dynamic_cast<T *>(list->FindObject(name));
        }

        std::string buildDirname(const std::string_view trigger, int R) {
            std::stringstream dirnamebuilder;
            dirnamebuilder << "JetSpectrum_" << fJetType << "_R" << std::setw(2) << std::setfill('0') << R << "_" << trigger;
            if(fSysVar.length()) {
                dirnamebuilder << "_" << fSysVar;
            }
            return dirnamebuilder.str();
        }

        TList *getHistos(const std::string_view trigger, int R) {
            fReader->cd(buildDirname(trigger, R).data());
            return static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        }
        
        void buildDatasets() {
            std::array<std::string, 3> triggers = {{"INT7", "EJ1", "EJ2"}};
            for(auto R : ROOT::TSeqI(2, 7)) {
                for(const auto &trg : triggers){
                    auto histos = getHistos(trg, R);
                    auto normhist = HistGetter<TH1>(histos, "fNormalisationHist"),
                         runcounter = HistGetter<TH1>(histos, "hEventCounterRun"),
                         counterAbs = HistGetter<TH1>(histos, "hClusterCounterAbs"),
                         counterWeighted = HistGetter<TH1>(histos, "hClusterCounter");
                    auto specAbs = HistGetter<TH2>(histos, "hJetSpectrumAbs"),
                         specWeighted = HistGetter<TH2>(histos, "hJetSpectrum");
                    fDatasets[Key(R, trg)] = Dataset(normhist, runcounter, counterAbs, counterWeighted, specAbs, specWeighted);
                }
            }
            for(auto &dset : fDatasets) dset.second.setPeriodHandler(&fPeriodHandler);
            std::cout << "DataFileHandler: Found " << fDatasets.size() << " datasets" << std::endl;
        }

        void buildLuminosityHandler() {
            TList *luminosityHistos(nullptr);
            for(auto keyiter : TRangeDynCast<TKey>(fReader->GetListOfKeys())) {
                std::string_view keyname(keyiter->GetName());
                if(keyname.find("EmcalTriggerNorm") != std::string::npos) {
                    fReader->cd(keyname.data());
                    luminosityHistos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
                }
            }
            if(luminosityHistos) fLuminosityHandler = std::make_unique<PWG::EMCAL::AliEmcalTriggerLuminosity>(luminosityHistos);
            fLuminosityHandler->Evaluate();
        }

        std::unique_ptr<TFile> fReader;
        std::string fJetType;
        std::string fSysVar;
        std::map<Key, Dataset> fDatasets;
        PeriodHandler fPeriodHandler;
        std::unique_ptr<PWG::EMCAL::AliEmcalTriggerLuminosity> fLuminosityHandler;
};
#endif // DATAFILEHANDLER_H