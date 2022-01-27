#ifndef TRIGGEREFFICIENCY_CXX
#define TRIGGEREFFICIENCY_CXX

#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "RawDataSpectrum.cxx"

class TriggerEfficiency {
public:
    TriggerEfficiency() = default;
    TriggerEfficiency(const RawDataSpectrum &minbias, const RawDataSpectrum &triggered, const std::vector<double> detlevelbinning) { build(minbias, triggered, detlevelbinning); }
    ~TriggerEfficiency() = default;

    TH1 *getEfficiencyFine() const { if(!fEfficiencyFine) throw UninitializedException(); return fEfficiencyFine; }
    TH1 *getEfficiencyRebinned() const { if(!fEfficiencyRebinned) throw UninitializedException(); return fEfficiencyRebinned; }

private:
    void build(const RawDataSpectrum &minbias, const RawDataSpectrum &triggered, const std::vector<double> detlevelbinning){
        const auto MCtriggercluster = TriggerCluster::kANY;
        std::unique_ptr<TH1> histminbias(minbias.getSpectrumForTriggerCluster(MCtriggercluster)),
                             mbrebinned(histminbias->Rebin(detlevelbinning.size() - 1, "mcrebinned", detlevelbinning.data())); 
        fEfficiencyFine = triggered.getSpectrumForTriggerCluster(MCtriggercluster);
        fEfficiencyRebinned = fEfficiencyFine->Rebin(detlevelbinning.size() - 1, "triggerEfficiencyRebinned", detlevelbinning.data());
        fEfficiencyFine->SetDirectory(nullptr);
        fEfficiencyFine->SetName("triggerEfficiency");
        fEfficiencyFine->Divide(fEfficiencyFine, histminbias.get(), 1., 1., "b");
        fEfficiencyRebinned->SetDirectory(nullptr);
        fEfficiencyRebinned->Divide(fEfficiencyRebinned, mbrebinned.get(), 1., 1., "b");
    }

    TH1 *fEfficiencyFine = nullptr;
    TH1 *fEfficiencyRebinned = nullptr;
};

#endif