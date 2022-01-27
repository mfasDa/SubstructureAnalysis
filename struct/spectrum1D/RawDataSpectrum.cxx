#ifndef RAWDATASPECTRUM
#define RAWDATASPECRTUM

#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../HistogramDataHandler.cxx"
#include "TriggerDefinition.cxx"

class RawDataSpectrum : public TriggerClusterBinned, public UnmanagedHistogramData<const TH2> {
    public:
        RawDataSpectrum() = default;
        RawDataSpectrum(const TH2 *histogram) : UnmanagedHistogramData<const TH2>(histogram) {}
        ~RawDataSpectrum() = default;

        TH1 *getSpectrumForTriggerCluster(TriggerCluster clust) const {
            auto data = getHistogram();
            TH1 *result{nullptr};
            int bin = getClusterBin(clust);
            const std::map<TriggerCluster, std::string> clustertitle = {{TriggerCluster::kANY, "ANY"}, {TriggerCluster::kCENT, "CENT"}, {TriggerCluster::kCENTNOTRD, "CENTNOTRD"}};
            result = data->ProjectionY(Form("%s_%s", data->GetName(), clustertitle.find(clust)->second.data()), bin, bin);
            result->SetDirectory(nullptr);
            return result;
        }
};
#endif