#include "../meta/stl.C"
#include "../meta/root.C"
#include "../helpers/root.C"

enum class TriggerCluster {
    kANY,
    kCENT,
    kCENTNOTRD
};

class UninitException : public std::exception{
public:
    UninitException() = default;
    ~UninitException() noexcept = default;

    const char *what() const noexcept { return "Data uninitialized"; }
};

class TriggerClusterBinned {
    public:
        TriggerClusterBinned() = default;
        ~TriggerClusterBinned() = default;

        int getClusterBin(TriggerCluster clust) const {
            switch (clust) {
                case TriggerCluster::kANY: return 1;
                case TriggerCluster::kCENT: return 2;
                case TriggerCluster::kCENTNOTRD: return 3;
            };
        }
};

class RawDataSpectrum : public TriggerClusterBinned {
    public:
        RawDataSpectrum() = default;
        RawDataSpectrum(const TH2 *histogram) : fData(histogram) {}
        ~RawDataSpectrum() = default;

        TH1 *getSpectrumForTriggerCluster(TriggerCluster clust) const {
            if(!fData) throw UninitException();
            TH1 *result{nullptr};
            int bin = getClusterBin(clust);
            const std::map<TriggerCluster, std::string> clustertitle = {{TriggerCluster::kANY, "ANY"}, {TriggerCluster::kCENT, "CENT"}, {TriggerCluster::kCENTNOTRD, "CENTNOTRD"}};
            result = fData->ProjectionY(Form("%s_%s", fData->GetName(), clustertitle.find(clust)->second.data()), bin, bin);
            result->SetDirectory(nullptr);
            return result;
        }

        void setHistogram(const TH2 *hist) { fData = hist; }

    private:
        const TH2 *fData = nullptr;
};

class ClusterCounter : public TriggerClusterBinned {
    public:
        ClusterCounter() = default;
        ClusterCounter(const TH1 *hist) : fData(hist) {}
        ~ClusterCounter() = default;

        double getCounters(TriggerCluster clust) const {
            if(!fData) throw UninitException();
            return fData->GetBinContent(getClusterBin(clust));
        }

        double makeClusterRatio(TriggerCluster clusterNum, TriggerCluster clusterDen) const {
            return getCounters(clusterNum) / getCounters(clusterDen);
        }
        
        void setHistogram(const TH1 *hist) { fData = hist; }
    private:
        const TH1 *fData = nullptr;
};

class VertexFindingEfficiency {
    public:
        VertexFindingEfficiency() = default;
        VertexFindingEfficiency(const TH1 *normalizationHist): fNormalizationHist(std::unique_ptr<TH1>(histcopy(normalizationHist))) {}
        ~VertexFindingEfficiency() = default;

        operator double() const { return evaluate(); }

        double evaluate() const {
            if(!fNormalizationHist) throw UninitException();
            return fNormalizationHist->GetBinContent(fNormalizationHist->GetXaxis()->FindBin("Vertex reconstruction and quality")) / fNormalizationHist->GetBinContent(fNormalizationHist->GetXaxis()->FindBin("Event selection"));
        }
        
        void setHistogram(const TH1 *hist) { fNormalizationHist = std::shared_ptr<TH1>(histcopy(hist)); }

    private:
        std::shared_ptr<TH1> fNormalizationHist;
};


class DataFileHandler {
    public:
        class Dataset {
            public:
                Dataset() = default;
                ~Dataset() = default;
                Dataset(const TH1 *normalizationHist, const TH1 *counterAbs, const TH1 *counterWeighted, const TH2 *spectrumAbs, const TH2 *spectrumWeighted) :
                    fEfficiencyVtx(normalizationHist),
                    fClusterCounterAbs(counterAbs),
                    fClusterCounterWeighted(counterWeighted),
                    fSpectrumAbs(spectrumAbs),
                    fSpectrumWeighted(spectrumWeighted)
                {
                }

                const VertexFindingEfficiency &getVertexFindingEfficiency() const { return fEfficiencyVtx; }
                const ClusterCounter &getAbsCounters() const { return fClusterCounterAbs; }
                const ClusterCounter &getWeightedCounters() const { return fClusterCounterWeighted; }
                const RawDataSpectrum &getRawSpectrumAbs() const { return fSpectrumAbs; }
                const RawDataSpectrum &getRawSpectrumWeighted() const { return fSpectrumWeighted; }

                void setNormalizationHistogram(const TH1 *normalizationHist) { fEfficiencyVtx.setHistogram(normalizationHist); }
                void setAbsCounters(const TH1 *abscounters) { fClusterCounterAbs.setHistogram(abscounters); }
                void setWeightedCounters(const TH1 *weightedcounters) { fClusterCounterWeighted.setHistogram(weightedcounters); }
                void setAbsSpectrum(const TH2 *absspectrum) { fSpectrumAbs.setHistogram(absspectrum); }
                void setWeightedSpectrum(const TH2 *weightedspectrum) { fSpectrumWeighted.setHistogram(weightedspectrum); }


            private:
                VertexFindingEfficiency fEfficiencyVtx;
                ClusterCounter fClusterCounterAbs;
                ClusterCounter fClusterCounterWeighted;
                RawDataSpectrum fSpectrumAbs;
                RawDataSpectrum fSpectrumWeighted;
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
            return dynamic_cast<T>(list->FindObject(name));
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
                    auto normhist = HistGetter<TH1>(histos, ""),
                         counterAbs = HistGetter<TH1>(histos, ""),
                         counterWeighted = HistGetter<TH1>(histos, "");
                    auto specAbs = HistGetter<TH2>(histos, ""),
                         specWeighted = HistGetter<TH2>(histos, "");
                    fDatasets[Key(R, trg)] = Dataset(normhist, counterAbs, counterWeighted, specAbs, specWeighted);
                }
            }
        }

        std::unique_ptr<TFile> fReader;
        std::string fJetType;
        std::string fSysVar;
        std::map<Key, Dataset> fDatasets;
};