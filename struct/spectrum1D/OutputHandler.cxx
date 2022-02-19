#ifndef __OUTPUTHANDLER_CXX__
#define __OUTPUTHANDLER_CXX__

#include "../../meta/root.C"
#include "../../meta/stl.C"

class OutputHandler {
public:
    struct DataContent {
        struct RawEventCounter {
            std::string mTrigger;
            int mYear;
            unsigned long mNumEvents;
        };
        std::map<std::string, TH1 *> mRawHist;
        std::map<std::string, TH1 *> mNormalizedRawHist;
        std::map<std::string, TH1 *> mNormalizedRawhistRebinned;
        std::map<std::string, TH1 *> mTriggerEffFine;
        std::map<std::string, TH1 *> mTriggerEffRebinned;
        std::map<std::string, TH1 *> mRawHistCorrected;
        TH1 *mCombinedRawHistogram = nullptr;
        TH1 *mCombinedRawHistogramCorrected = nullptr;
        std::map<int, double> mVertexFindingEfficiency;
        std::map<int, double> mCENTNOTRDCorrection;
        TH1 *mPurity = nullptr;
        TH1 *mLuminosityAllYears = nullptr;
        std::map<int, TH1 *> mLuminosityYears;
        std::map<std::string, TH1 *> mLuminosityTriggers;
        std::vector<RawEventCounter> mEventCounters;

        void setRawEventCounts(int year, const std::string_view trigger, unsigned long eventcounts) {
            auto found = std::find_if(mEventCounters.begin(), mEventCounters.end(), [year, trigger](const RawEventCounter count) { return year == count.mYear && trigger == count.mTrigger; });
            if(found != mEventCounters.end()) found->mNumEvents = eventcounts;
            else mEventCounters.push_back(RawEventCounter{trigger.data(), year, eventcounts});
        }

        void write() {
            auto base = gDirectory;
            base->mkdir("rawlevel");
            base->cd("rawlevel");
            for(auto &[trg, hist] : mRawHist) hist->Write();
            for(auto &[trg, hist] : mNormalizedRawHist) hist->Write();
            for(auto &[trg, hist] : mNormalizedRawHist) hist->Write();
            if(mCombinedRawHistogram) mCombinedRawHistogram->Write();
            if(mCombinedRawHistogramCorrected) mCombinedRawHistogramCorrected->Write();
            auto histEffVtx = buildHistVertexFindingEfficiency();
            if(histEffVtx) histEffVtx->Write();
            auto histCENTNOTRD = buildHistsCENTNOTRDCorrection();
            if(histCENTNOTRD) histCENTNOTRD->Write();
            if(mPurity) mPurity->Write();
            if(mLuminosityAllYears) mLuminosityAllYears->Write();
            for(auto &[year, lumi] : mLuminosityYears) lumi->Write();
            for(auto &[trg, lumi] : mLuminosityTriggers) lumi->Write();
            for(auto evcounter : buildEventCounterHistsYears()) evcounter->Write();
            for(auto evcounter : buildEventCounterHistsTriggers()) evcounter->Write();
            base->cd();
        }

        TH1 *buildHistVertexFindingEfficiency(){
            if(!mVertexFindingEfficiency.size()) return nullptr;
            int yearmin = INT_MAX,
                yearmax = INT_MIN;
            for(auto &[year, eff] : mVertexFindingEfficiency) {
                if(year < yearmin) yearmin = year;
                if(year > yearmax) yearmax = year;
            }
            double minrange = yearmin - 0.5, maxrange = yearmax + 0.5;
            int nbins = int(maxrange - minrange);
            auto heffVtx = new TH1F("hVertexFindingEfficiency", "Vertex finding efficiency; year; #epsilon_{vtx}", nbins, minrange, maxrange);
            heffVtx->SetDirectory(nullptr);
            for(auto &[year, eff] : mVertexFindingEfficiency) {
                heffVtx->SetBinContent(heffVtx->GetXaxis()->FindBin(year), eff);
            }
            return heffVtx;
        }
        
        TH1 *buildHistsCENTNOTRDCorrection(){
            if(!mCENTNOTRDCorrection.size()) return nullptr;
            int yearmin = INT_MAX,
                yearmax = INT_MIN;
            for(auto &[year, eff] : mCENTNOTRDCorrection) {
                if(year < yearmin) yearmin = year;
                if(year > yearmax) yearmax = year;
            }
            double minrange = yearmin - 0.5, maxrange = yearmax + 0.5;
            int nbins = int(maxrange - minrange);
            auto heffVtx = new TH1F("hCENTNOTRDcorrection", "CENTNOTRD correction; year; c_{CENTNOTRD}", nbins, minrange, maxrange);
            heffVtx->SetDirectory(nullptr);
            for(auto &[year, corr] : mCENTNOTRDCorrection) {
                heffVtx->SetBinContent(heffVtx->GetXaxis()->FindBin(year), corr);
            }
            return heffVtx;
        }

        std::vector<TH1 *> buildEventCounterHistsYears() {
            std::vector<TH1 *> result;
            std::map<int, std::set<std::string>> database;
            for(const auto &count : mEventCounters) {
                database[count.mYear].insert(count.mTrigger);
            }
            for(const auto &[year, triggers] : database) {
                auto absyear = year;
                if(!triggers.size()) continue;
                auto hist = new TH1D(Form("hEventCounterAbs%d", year), Form("Abs. event counter for %d; trigger; Number of events", year), triggers.size(), 0, triggers.size());
                hist->SetDirectory(nullptr);
                int currentbin = 1;
                for(auto trg : triggers) {
                    hist->GetXaxis()->SetBinLabel(currentbin, trg.data());
                    auto rawcounter = std::find_if(mEventCounters.begin(), mEventCounters.end(), [absyear, trg](RawEventCounter &counts) { return absyear == counts.mYear && trg == counts.mTrigger; });
                    hist->SetBinContent(currentbin, rawcounter->mNumEvents);
                    currentbin++;
                }
                result.push_back(hist);
            }
            return result;
        }

        std::vector<TH1 *> buildEventCounterHistsTriggers() {
            std::vector<TH1 *> result;
            std::map<std::string, std::set<int>> database;
            for(const auto &count : mEventCounters) {
                database[count.mTrigger].insert(count.mYear);
            }
            for(const auto &[trg, years] : database) {
                if(!years.size()) continue;
                int yearmin = INT_MAX,
                    yearmax = INT_MIN;
                for(auto year : years) {
                    if(year < yearmin) yearmin = year;
                    if(year > yearmax) yearmax = year;
                }
                double minrange = yearmin - 0.5, maxrange = yearmax + 0.5;
                int nbins = int(maxrange - minrange);
                auto hist = new TH1D(Form("hEventCounterAbs%Trigger", trg.data()), Form("Abs. event counter for %s; year; Number of events", trg.data()), nbins, minrange, maxrange);
                hist->SetDirectory(nullptr);
                std::string_view mytrigger = trg;
                for(auto year: years) {
                    auto rawcounter = std::find_if(mEventCounters.begin(), mEventCounters.end(), [year, mytrigger](RawEventCounter &counts) { return year == counts.mYear && mytrigger == counts.mTrigger; });
                    hist->SetBinContent(hist->GetXaxis()->FindBin(year), rawcounter->mNumEvents);
                }
                result.push_back(hist);
            }
            return result;

        }
    };

    struct MCContent {
        TH1 *mScale = nullptr;
        TH2 *mResponseMatrixFine = nullptr;
        TH2 *mResponseMatrixRebinned = nullptr;
        TH1 *mKinematicEfficiency = nullptr;
        TH1 *mJetFindingEfficency = nullptr;
        TH1 *mTruefull = nullptr;
        TH1 *mTruefullAll = nullptr;

        void write() {
            auto base = gDirectory;
            base->mkdir("response");
            base->cd("response");
            mScale->Write();
            if(mResponseMatrixFine) mResponseMatrixFine->Write();
            if(mResponseMatrixRebinned) mResponseMatrixRebinned->Write();
            if(mKinematicEfficiency) mKinematicEfficiency->Write();
            if(mJetFindingEfficency) mJetFindingEfficency->Write();
            if(mTruefull) mTruefull->Write();
            if(mTruefullAll) mTruefullAll->Write("partall");
            base->cd();
        }
    };

    struct ClosureContent {
        TH1 *mPriorsClosure = nullptr;
        TH1 *mDetClosureOrig = nullptr;
        TH1 *mDetClosure = nullptr;
        TH1 *mPartClosure = nullptr;
        TH1 *mTruefullClosure = nullptr;
        TH2 *mResponseClosure = nullptr;
        TH2 *mRebinnedResponseClosure = nullptr;
        TH1 *mJetFindingPurityClosure = nullptr;
        TH1 *mJetFindingEfficiencyClosure = nullptr;

        void write(){
            auto base = gDirectory;
            base->mkdir("closuretest");
            base->cd("closuretest");
            if(mPriorsClosure) mPriorsClosure->Write("priorsclosure");
            if(mDetClosureOrig) mDetClosureOrig->Write("detclosureFull");
            if(mDetClosure) mDetClosure->Write("detclosure");
            if(mPartClosure) mPartClosure->Write("partclosure");
            if(mTruefullClosure) mTruefullClosure->Write("partallclosure");
            if(mResponseClosure) mResponseClosure->Write("responseClosureFine");
            if(mRebinnedResponseClosure) mRebinnedResponseClosure->Write("responseClosureRebinned");
            if(mJetFindingPurityClosure) mJetFindingPurityClosure->Write();
            if(mJetFindingEfficiencyClosure) mJetFindingEfficiencyClosure->Write();
            base->cd();
        }
    };

    struct UnfoldedHists {
        int mRegularization;
        TH1 *mUnfolded;
        TH1 *mNormalized;
        TH1 *mFullyCorrected;
        TH1 *mBackfolded;
        TH2 *mPearson;
        TH1 *mDvector;
        TH1 *mUnfoldedClosure;
        TH1 *mFullyCorrectedClosure;
        TH2 *mPearsonClosure;
        TH1 *mDvectorClosure;

        void write() {
            auto base = gDirectory;
            std::string regstring = Form("reg%d", mRegularization);
            base->mkdir(regstring.data());
            base->cd(regstring.data());
            if(mUnfolded) mUnfolded->Write();
            if(mNormalized) mNormalized->Write();
            if(mFullyCorrected) mFullyCorrected->Write();
            if(mBackfolded) mBackfolded->Write();
            if(mPearson) mPearson->Write();
            if(mDvector) mDvector->Write();
            if(mUnfoldedClosure) mUnfoldedClosure->Write();
            if(mFullyCorrectedClosure) mFullyCorrectedClosure->Write();
            if(mPearsonClosure) mPearsonClosure->Write();
            if(mDvectorClosure) mDvectorClosure->Write();
            base->cd();
        }
    };

    struct RadiusContent {
        int mR;
        DataContent mData;
        MCContent mMC;
        ClosureContent mClosure;
        std::map<int, UnfoldedHists> mUnfolded;

        void setUnfoldingResults(int reg, TH1 *unfolded, TH1 *normalized, TH1 *fullyCorrected,  TH1 *backfolded, TH2 *pearson, TH1 *dvector, TH1 *unfoldedClosure,TH1 *fullyCorrectedClosure, TH2 *pearsonClosure, TH1 *dvectorClosure) {
            mUnfolded[reg] = {reg, unfolded, normalized, fullyCorrected, backfolded, pearson, dvector, unfoldedClosure, fullyCorrectedClosure, pearsonClosure, dvectorClosure};
        }

        void write() {
            auto base = gDirectory;
            std::string rstring = Form("R%02d", mR);
            base->mkdir(rstring.data());
            base->cd(rstring.data());
            mData.write();
            mMC.write();
            mClosure.write();
            for(auto &[ref, hists] : mUnfolded) hists.write();
            base->cd();
        }
    };

    OutputHandler() = default;
    ~OutputHandler() = default;

    void setRawHistTrigger(int R, TH2 *hist, const std::string_view trg) {
        mRdata[R].mData.mRawHist[trg.data()] = hist;
    }

    void setNormalizedRawSpectrumTrigger(int R, TH2 *hist, const std::string_view trg, bool rebinned) {
        auto &rdata = mRdata[R];
        auto &datacontent = rdata.mData;
        if(rebinned) datacontent.mNormalizedRawhistRebinned[trg.data()] = hist;
        else datacontent.mNormalizedRawHist[trg.data()] = hist;
    }

    void setTrgEffCorrectedRawSpectrumTrigger(int R, TH2 *hist, const std::string_view trg) {
        mRdata[R].mData.mRawHistCorrected[trg.data()] = hist;
    }

    void setTriggerEff(int R, TH1 *hist, const std::string_view trg, bool rebinned) {
        auto &rdata = mRdata[R];
        auto &datacontent = rdata.mData;
        if(rebinned) datacontent.mTriggerEffRebinned[trg.data()] = hist;
        else datacontent.mTriggerEffFine[trg.data()] = hist;
    }

    void setCombinedRawSpectrum(int R, TH1 *hist, bool purityCorrected) {
        auto &rdata = mRdata[R];
        auto &datacontent = rdata.mData;
        if(purityCorrected) datacontent.mCombinedRawHistogramCorrected = hist;
        else datacontent.mCombinedRawHistogramCorrected = hist;
    }

    void setCombinedLuminosity(int R, TH1 *hist){ 
        mRdata[R].mData.mLuminosityAllYears = hist;
    }

    void setLuminosityTriggerClasses(int R, const std::map<std::string, TH1 *> histos) { 
        mRdata[R].mData.mLuminosityTriggers = histos;
    }
    
    void setLuminosityYears(int R, const std::map<int, TH1 *> histos){ 
        mRdata[R].mData.mLuminosityYears = histos;
    }

    void setVertexFindingEfficiency(int R, int year, double eff) {
        mRdata[R].mData.mVertexFindingEfficiency[year] = eff;
    }

    void setCENTNOTRDCorrection(int R, int year, double correction) {
        mRdata[R].mData.mCENTNOTRDCorrection[year] = correction;
    }

    void setRawEvents(int R, int year, const std::string_view trigger, unsigned long eventcounts){
        mRdata[R].mData.setRawEventCounts(year, trigger, eventcounts);
    }

    void setResponseMatrix(int R, TH2 *hist, bool fine) {
        auto &rdata = mRdata[R];
        auto &mccontent = rdata.mMC;
        if(fine) mccontent.mResponseMatrixFine = hist;
        else mccontent.mResponseMatrixRebinned = hist;
    }

    void setResponseMatrixClosure(int R, TH2 *hist, bool fine) {
        auto &rdata = mRdata[R];
        auto &closurecontent = rdata.mClosure;
        if(fine) closurecontent.mResponseClosure= hist;
        else closurecontent.mRebinnedResponseClosure = hist;
    }

    void setMCTruth(int R, TH1 *hist, bool full) {
        auto &rdata = mRdata[R];
        auto &mccontent = rdata.mMC;
        if(full) mccontent.mTruefullAll = hist;
        else mccontent.mTruefull = hist;
    }

    void setMCScale(int R, double mcscale) {
        auto hmcscale = new TH1F("hMCscale", "MC scale", 1, 0.5, 1.5);
        hmcscale->SetDirectory(nullptr);
        hmcscale->SetBinContent(1, mcscale);
        hmcscale->Write();
        mRdata[R].mMC.mScale = hmcscale;
    }

    void setKinematicEfficiency(int R, TH1 *hist) {
        mRdata[R].mMC.mKinematicEfficiency = hist;
    }

    void setJetFindingEfficiency(int R, TH1 *hist, bool closure) {
        auto &rdata = mRdata[R];
        if(closure) {
            auto &closurecontent = rdata.mClosure;
            closurecontent.mJetFindingEfficiencyClosure = hist;
        } else {
            auto &mccontent = rdata.mMC;
            mccontent.mJetFindingEfficency = hist;
        }
    }

    void setJetFindingPurity(int R, TH1 *hist, bool closure) {
        auto &rdata = mRdata[R];
        if(closure) {
            auto &closurecontent = rdata.mClosure;
            closurecontent.mJetFindingPurityClosure = hist;
        } else {
            auto &datacontent = rdata.mData;
            datacontent.mPurity = hist;
        }
    }

    void setDetLevelClosure(int R, TH1 *hist, bool purityCorrected) { 
        auto &closuredata = mRdata[R].mClosure;
        if(purityCorrected) closuredata.mDetClosure = hist;
        else closuredata.mDetClosureOrig = hist;
    }
    
    void setPartLevelClosure(int R, TH1 *hist, bool full) { 
        auto &closuredata = mRdata[R].mClosure;
        if(full) closuredata.mTruefullClosure = hist;
        else closuredata.mPartClosure = hist;
    }

    void setPriorsClosure(int R, TH1 *hist) {
        mRdata[R].mClosure.mPriorsClosure = hist;
    }

    void setUnfoldingResults(int R, int reg, TH1 *unfolded, TH1 *normalized, TH1 *fullycorrected,  TH1 *backfolded, TH2 *pearson, TH1 *dvector, TH1 *unfoldedClosure, TH1 *fullyCorrectedClosure, TH2 *pearsonClosure, TH1 *dvectorClosure) {
       mRdata[R].setUnfoldingResults(reg, unfolded, normalized, fullycorrected, backfolded, pearson, dvector, unfoldedClosure, fullyCorrectedClosure, pearsonClosure, dvectorClosure);
    }

    void write(const std::string_view outputfile) {
        std::unique_ptr<TFile> writer(TFile::Open(outputfile.data(), "RECREATE"));
        for(auto &[R, data] : mRdata) data.write();
    }

private:
    std::map<int, RadiusContent> mRdata;
};

#endif