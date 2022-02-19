#ifndef MCFILEHANDLER_H
#define MCFILEHANDLER_H

#include "../../meta/stl.C"
#include "../../meta/root.C"

#include "RawDataSpectrum.cxx"
#include "../HistogramDataHandler.cxx"

class Trials : public ManagedHistogramData<TH1> {
public:
    Trials() = default;
    Trials(const TH1 *hist) : ManagedHistogramData<TH1>(hist) { }
    ~Trials() = default;

    double getMaxTrials() const {
        auto bins = getBins();
        return *std::max_element(bins.begin(), bins.end());
    }

    double getAverageTrials() const {
        auto bins = getBins();
        return TMath::Mean(bins.begin(), bins.end());
    }

    double getTrialsFit() const {
        auto hist = getHistogram();
        TF1 model("meanntrials", "pol0", 0., 100.);
        hist->Fit(&model, "N", "", hist->GetXaxis()->GetBinLowEdge(2), hist->GetXaxis()->GetBinUpEdge(hist->GetXaxis()->GetNbins()+1));
        return model.GetParameter(0);
    }

private:
    std::vector<double> getBins() const {
        auto hist = getHistogram();
        std::vector<double> result;
        for(int ib = 0; ib < hist->GetXaxis()->GetNbins(); ib++) {
            auto entries  = hist->GetBinContent(ib+1);
            if(TMath::Abs(entries) > DBL_EPSILON) {
                // filter 0
                result.emplace_back(entries);
            }
        }
        return result;
    }
};


class RawResponseMatrix : public UnmanagedHistogramData<TH2> {
public:
    RawResponseMatrix() = default;
    RawResponseMatrix(TH2 *histogram) : UnmanagedHistogramData(histogram) {};
    ~RawResponseMatrix() = default;

    TH2 *getRawResponse() const { return getHistogram(); };
    void Scale(double scale) { getHistogram()->Scale(scale); }
};

class JetFindingEfficiency : public ManagedHistogramData<TH2> {
public:
    enum {
        kNotReconstructed = 1, 
        kReconstructedNoAcceptance = 2,
        kAccepted = 3
    };
    JetFindingEfficiency() = default;
    JetFindingEfficiency(const TH2 *hist) : ManagedHistogramData<TH2>(hist) {}

    TH1 *makeEfficiency(const std::vector<double> partptbinning) const {
        auto hist = getHistogram();
        std::unique_ptr<TH1> allraw(hist->ProjectionX("allraw")),
                             recraw(hist->ProjectionX("recraw", kAccepted, kAccepted)),
                             all(allraw->Rebin(partptbinning.size() - 1, "allrawRebinned", partptbinning.data()));
        auto efficiency = recraw->Rebin(partptbinning.size() - 1, "jetfindingEfficiency", partptbinning.data());
        efficiency->SetDirectory(nullptr);
        efficiency->Divide(efficiency, all.get(), 1., 1., "b");
        return efficiency;
    }
};

class JetFindingPurity : public ManagedHistogramData<TH2>{
public:
    enum {
        kNotMatched = 1, 
        kMatchedNoAcceptance = 2,
        kAccepted = 3
    };
    JetFindingPurity() = default;
    JetFindingPurity(const TH2 *hist) : ManagedHistogramData<TH2>(hist) {}

    TH1 *makePurity(const std::vector<double> detptbinning) const {
        auto hist = getHistogram();
        std::unique_ptr<TH1> allraw(hist->ProjectionX("allraw")),
                             recraw(hist->ProjectionX("recraw", kAccepted, kAccepted)),
                             all(allraw->Rebin(detptbinning.size() - 1, "allrawRebinned", detptbinning.data()));
        auto purity = recraw->Rebin(detptbinning.size() - 1, "jetfindingPurity", detptbinning.data());
        purity->SetDirectory(nullptr);
        purity->Divide(purity, all.get(), 1., 1., "b");
        return purity;
    }
};

class MCFileHandler {
public:
    enum TriggerClass {
        INT7 = 0,
        EJ2 = 1,
        EJ1 = 2
    };
    class RadiusNotFoundException : public std::exception{
    public:
        RadiusNotFoundException(int R) : fRadius(R), fMessage() {
            std::stringstream msgbuilder;
            msgbuilder << "Radius not found: " << fRadius;
            fMessage = msgbuilder.str();
        }
        ~RadiusNotFoundException() noexcept = default;

        const char *what() const noexcept { return fMessage.data(); }
        int getR() const noexcept { return fRadius; }

    private:
        int fRadius;
        std::string fMessage;
    }; 

    class InputNotFoundException : public std::exception {
    public:
        InputNotFoundException(int R, const std::string_view input) : fR(R), fInput(input), fMessage() {
            std::stringstream msgbuilder;
            msgbuilder << "Input not found R " << R << ": " << input;
            fMessage = msgbuilder.str();
        }
        ~InputNotFoundException() noexcept = default;

        const char *what() const noexcept { return fMessage.data(); }
        const std::string_view getInput() const noexcept { return fInput; }
        int getR() const noexcept { return fR; }
    private:
        int fR;
        std::string fInput;
        std::string fMessage;
    };

    class MCSet{
    public:
        using PartLevelSpectrum = UnmanagedHistogramData<TH1>;
        class TriggerNotFoundException: public std::exception{ 
        public:
            TriggerNotFoundException(const std::string_view triggerclass) : fTrigger(triggerclass), fMessage() {
                std::stringstream msgbuilder;
                msgbuilder << "Trigger not found: " << fTrigger;
                fMessage = msgbuilder.str();
            }
            ~TriggerNotFoundException() noexcept = default;

            const char *what() const noexcept { return fMessage.data(); }
            const std::string_view getTrigger() const noexcept { return fTrigger; }

        private:
            std::string fTrigger;
            std::string fMessage;
        };

        MCSet() = default;
        MCSet(TH2 *response, TH2 *jetfindingEff, TH2 *jetfindingPurity, TH1 *partlevelSpectrum, const TH1 *ntrials) :
            fResponse(response),
            fJetFindingEfficiency(jetfindingEff),
            fJetFindingPurity(jetfindingPurity),
            fPartLevelSpectrum(partlevelSpectrum),
            fNTrials(ntrials)
        {
        }
        MCSet(TH2 *response, TH2 *jetfindingEff, TH2 *jetfindingPurity, TH1 *partlevelSpectrum, const TH1 *ntrials, TH2 *detINT7, TH2 *detEJ2, TH2 *detEJ1) :
            fResponse(response),
            fJetFindingEfficiency(jetfindingEff),
            fJetFindingPurity(jetfindingPurity),
            fPartLevelSpectrum(partlevelSpectrum),
            fNTrials(ntrials)
        {
            fDetLevelSpectra[TriggerClass::INT7] = RawDataSpectrum(detINT7);
            fDetLevelSpectra[TriggerClass::EJ2] = RawDataSpectrum(detEJ2);
            fDetLevelSpectra[TriggerClass::EJ1] = RawDataSpectrum(detEJ1);
        }
        ~MCSet() = default;

        RawResponseMatrix &getResponseMatrix() { return fResponse; }
        JetFindingEfficiency &getJetFindingEfficiency() { return fJetFindingEfficiency; }
        JetFindingPurity &getJetFindingPurity() { return fJetFindingPurity; }
        PartLevelSpectrum &getPartLevelSpectrum() { return fPartLevelSpectrum; }
        RawDataSpectrum &getDetLevelSpectrum(TriggerClass trigger) { 
            auto found = fDetLevelSpectra.find(trigger);
            if(found == fDetLevelSpectra.end()) throw TriggerNotFoundException(getTriggerName(trigger));
            return found->second;
        }
        Trials &getTrials() { return fNTrials; }

        void setPartLevelSpectrum(TH1 *partLevelSpectrum) { fPartLevelSpectrum.setHistogram(partLevelSpectrum); }
        void setDetLevelSpectrum(TH2 *spectrum, TriggerClass trigger) {
            auto found = fDetLevelSpectra.find(trigger);
            if(found == fDetLevelSpectra.end()) {
                fDetLevelSpectra[trigger] = RawDataSpectrum(spectrum);
            } else {
                found->second.setHistogram(spectrum);
            }
        }
        void setResponse(TH2 *response) { fResponse.setHistogram(response); }
        void setJetFindingEfficiency(TH2 *efficiency) { fJetFindingEfficiency.setHistogram(efficiency); }
        void setJetFindingPurity(TH2 *purity) { fJetFindingPurity.setHistogram(purity); }
        void setTrials(const TH1 *trials) { fNTrials.setHistogram(trials); }

    private:
        std::string getTriggerName(TriggerClass trigger) {
            switch(trigger){
                case TriggerClass::INT7: return "INT7";
                case TriggerClass::EJ2: return "EJ2";
                case TriggerClass::EJ1: return "EJ1";
            };
        }
        RawResponseMatrix fResponse;
        JetFindingEfficiency fJetFindingEfficiency;
        JetFindingPurity fJetFindingPurity;
        PartLevelSpectrum fPartLevelSpectrum;
        Trials fNTrials;
        std::map<TriggerClass, RawDataSpectrum> fDetLevelSpectra;
    };
    class ClosureSet{
    public:
        using ClosureFullTrueSpectrum = UnmanagedHistogramData<TH1>;
        ClosureSet() = default;
        ClosureSet(TH2 *response, TH2 *truth, TH2 *jetfindingEff, TH2 *jetfindingPurity, TH1 *partleveltrue, const TH1 *ntrials) :
            fResponse(response),
            fTruth(truth),
            fJetFindingEfficiency(jetfindingEff),
            fJetFindingPurity(jetfindingPurity),
            fPartLevelClosureTrue(partleveltrue),
            fNTrials(ntrials)
        { }
        ~ClosureSet() = default;

        RawResponseMatrix &getResponseMatrix() { return fResponse; }
        RawResponseMatrix &getTruthMatrix() { return fTruth; }
        JetFindingEfficiency &getJetFindingEfficiency() { return fJetFindingEfficiency; }
        JetFindingPurity &getJetFindingPurity() { return fJetFindingPurity; }
        ClosureFullTrueSpectrum &getPartLevelTrue() { return fPartLevelClosureTrue; }
        Trials &getTrials() { return fNTrials; }

        void setResponse(TH2 *response) { fResponse.setHistogram(response); }
        void setJetFindingEfficiency(TH2 *efficiency) { fJetFindingEfficiency.setHistogram(efficiency); }
        void setJetFindingPurity(TH2 *purity) { fJetFindingPurity.setHistogram(purity); }
        void setTrials(const TH1 *trials) { fNTrials.setHistogram(trials); }
        void setPartLevelClosureTrue(TH1 *spectrum) {fPartLevelClosureTrue.setHistogram(spectrum); }

    private:
        RawResponseMatrix fResponse;
        RawResponseMatrix fTruth;
        JetFindingEfficiency fJetFindingEfficiency;
        JetFindingPurity fJetFindingPurity;
        ClosureFullTrueSpectrum fPartLevelClosureTrue;
        Trials fNTrials;
    };

    MCFileHandler(const std::string_view filename, const std::string_view jettype, const std::string_view sysvar): 
        fReader(std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"))),
        fJetType(jettype),
        fSysVar(sysvar)
    { build(); }
     ~MCFileHandler() = default;

     MCSet &getMCSet(int R) {
         auto found = fMCsets.find(R);
         if(found == fMCsets.end()) throw RadiusNotFoundException(R);
         return found->second;
     }

     ClosureSet &getClosureSet(int R) {
         auto found = fClosureSets.find(R);
         if(found == fClosureSets.end()) throw RadiusNotFoundException(R);
         return found->second;
     }

private:
    void build() {
        std::map<int, std::vector<std::string>> directories;
        for(auto dir : TRangeDynCast<TKey>(fReader->GetListOfKeys())) {
            std::string_view dirname(dir->GetName());
            if((dirname.find("EnergyScaleResults") == std::string::npos) && (dirname.find("JetSpectrum") == std::string::npos)) continue;
            auto rstring = dirname.substr(dirname.find("R"), 3);
            auto R = std::atoi(rstring.substr(1, 2).data());
            auto found = directories.find(R);
            if(found == directories.end()){
                std::vector<std::string> rdirs = {dirname.data()};
                directories[R] = rdirs;
            } else {
                found->second.push_back(dirname.data());
            }
        }
        for(const auto &[R, rdirs] : directories) buildRadius(R, rdirs);
    }

    void buildRadius(int R, const std::vector<std::string> &directories) {
        std::string dirnameResponse;
        std::map<TriggerClass, std::string> dirnamesDet;
        for(const auto &dir: directories) {
            if(dir.find("EnergyScaleResults") != std::string::npos) dirnameResponse = dir;
            else dirnamesDet[getTriggerClassFromDirname(dir)] = dir;
        }
        if(!dirnameResponse.length()) throw InputNotFoundException(R, "EnergyScaleResults");

        fReader->cd(dirnameResponse.data());
        auto histlistResponse = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto response = HistGetter<TH2>(histlistResponse, "hJetResponseFine"),
             responseClosure = HistGetter<TH2>(histlistResponse, "hJetResponseFineClosure"),
             truthclosure = HistGetter<TH2>(histlistResponse, "hJetResponseFineNoClosure"),
             jetFindingEff = HistGetter<TH2>(histlistResponse, "hJetfindingEfficiencyCore"),
             jetFindingEffClosure = HistGetter<TH2>(histlistResponse, "hJetfindingEfficiencyCoreClosure"),
             jetFindingPure = HistGetter<TH2>(histlistResponse, "hPurityDet"),
             jetFindingPureClosure = HistGetter<TH2>(histlistResponse, "hPurityDetClosure");
        auto nTrials = HistGetter<TH1>(histlistResponse, "fHistTrials"), 
             partLevelSpectrum = HistGetter<TH1>(histlistResponse, "hJetSpectrumPartAll"),
             partLevelClosureTrue = HistGetter<TH1>(histlistResponse, "hJetSpectrumPartAllNoClosure");
        response->SetDirectory(nullptr);
        responseClosure->SetDirectory(nullptr);
        truthclosure->SetDirectory(nullptr);
        jetFindingEff->SetDirectory(nullptr);
        jetFindingEffClosure->SetDirectory(nullptr);
        jetFindingPure->SetDirectory(nullptr);
        jetFindingPureClosure->SetDirectory(nullptr);
        nTrials->SetDirectory(nullptr);
        partLevelSpectrum->SetDirectory(nullptr);
        std::map<TriggerClass, TH2 *> detLevelSpectra;
        for(const auto &[trigger, dirname]: dirnamesDet) {
            fReader->cd(dirname.data());
            auto histlistDet = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
            auto detspectrum = HistGetter<TH2>(histlistDet, "hJetSpectrum");
            detspectrum->SetDirectory(nullptr);
            detLevelSpectra[trigger] = detspectrum;
        }
        if(detLevelSpectra.size() == 3) {
            fMCsets[R] = MCSet(response, jetFindingEff, jetFindingPure, partLevelSpectrum, nTrials, detLevelSpectra[TriggerClass::INT7], detLevelSpectra[TriggerClass::EJ2], detLevelSpectra[TriggerClass::EJ1]);
        } else {
            fMCsets[R] = MCSet(response, jetFindingEff, jetFindingPure, partLevelSpectrum, nTrials, detLevelSpectra[TriggerClass::INT7], detLevelSpectra[TriggerClass::EJ2], detLevelSpectra[TriggerClass::EJ1]);
            auto found = fMCsets.find(R);
            for(auto &[trg, hist] : detLevelSpectra) found->second.setDetLevelSpectrum(hist, trg);
        }
        fClosureSets[R] = ClosureSet(responseClosure, truthclosure, jetFindingEffClosure, jetFindingPureClosure, partLevelClosureTrue, nTrials);
    }

    TriggerClass getTriggerClassFromDirname(const std::string_view triggername) {
        if(triggername.find("EJ1") != std::string::npos) return TriggerClass::EJ1;        
        else if(triggername.find("EJ2") != std::string::npos) return TriggerClass::EJ1;        
        else return TriggerClass::INT7;
    }

    template<typename T>
    T *HistGetter(TList *list, const char *name) {
        return dynamic_cast<T *>(list->FindObject(name));
    }

    std::unique_ptr<TFile> fReader;
    std::string fJetType;
    std::string fSysVar;
    std::map<int, MCSet> fMCsets;
    std::map<int, ClosureSet> fClosureSets;
};

#endif