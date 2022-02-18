#ifndef __OUTPUTHANDLER_CXX__
#define __OUTPUTHANDLER_CXX__

#include "../../meta/root.C"
#include "../../meta/stl.C"

class OutputHandler {
public:
    struct DataContent {
        std::map<std::string, TH1 *> mRawHist;
        std::map<std::string, TH1 *> mNormalizedRawHist;
        std::map<std::string, TH1 *> mNormalizedRawhistRebinned;
        std::map<std::string, TH1 *> mTriggerEffFine;
        std::map<std::string, TH1 *> mTriggerEffRebinned;
        std::map<std::string, TH1 *> mRawHistCorrected;
        TH1 *mCombinedRawHistogram = nullptr;
        TH1 *mCombinedRawHistogramCorrected = nullptr;
        TH1 *mVertexFindingEfficiency = nullptr;
        TH1 *mCENTNOTRDCorrection = nullptr;
        TH1 *mPurity = nullptr;
        TH1 *mLuminosityAllYears = nullptr;
        std::map<int, TH1 *> mEventCountersYears;
        std::map<int, TH1 *> mLuminosityYears;
        std::map<std::string, TH1 *> mLuminosityTriggers;

        void write() {
            auto base = gDirectory;
            base->mkdir("rawlevel");
            base->cd("rawlevel");
            for(auto &[trg, hist] : mRawHist) hist->Write();
            for(auto &[trg, hist] : mNormalizedRawHist) hist->Write();
            for(auto &[trg, hist] : mNormalizedRawHist) hist->Write();
            if(mCombinedRawHistogram) mCombinedRawHistogram->Write();
            if(mCombinedRawHistogramCorrected) mCombinedRawHistogramCorrected->Write();
            if(mVertexFindingEfficiency) mVertexFindingEfficiency->Write();
            if(mPurity) mPurity->Write();
            if(mLuminosityAllYears) mLuminosityAllYears->Write();
            for(auto &[year, counter]: mEventCountersYears) counter->Write();
            for(auto &[year, lumi] : mLuminosityYears) lumi->Write();
            for(auto &[trg, lumi] : mLuminosityTriggers) lumi->Write();
            base->cd();
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

    void setEventCountersYears(int R, const std::map<int, TH1 *> histos){ 
        mRdata[R].mData.mEventCountersYears = histos;
    }

    void setVertexFindingEfficiency(int R, double eff) {
        auto heffVtx = new TH1F("hVertexFindingEfficiency", "Vertex finding efficiency", 1, 0.5, 1.5);
        heffVtx->SetDirectory(nullptr);
        heffVtx->SetBinContent(1, eff);
        mRdata[R].mData.mVertexFindingEfficiency = heffVtx;
    }

    void setCENTNOTRDCorrection(int R, double correction) {
        auto hcntcorr = new TH1F("hCENTNOTRDcorrection", "CENTNOTRD correction", 1, 0.5, 1.5);
        hcntcorr->SetDirectory(nullptr);
        hcntcorr->SetBinContent(1, correction);
        mRdata[R].mData.mCENTNOTRDCorrection = hcntcorr;
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