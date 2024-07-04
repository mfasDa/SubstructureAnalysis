#ifndef __UNFOLDINGHANDLER_CXX_
#define __UNFOLDINGHANDLER_CXX_

#include "../../meta/root.C"
#include "../../meta/roounfold.C"
#include "../../meta/stl.C"

#include "../../helpers/root.C"
#include "../../helpers/math.C"
#include "../../helpers/unfolding.C"

struct UnfoldingResults {
    int fReg;
    TH1 *fUnfolded;
    TH1 *fNormalized;
    TH1 *fFullyCorrected;
    TH1 *fBackfolded;
    TH1 *fUnfoldedClosure;
    TH1 *fFullyCorrectedClosure;
    TH1 *fDvector;
    TH1 *fDvectorClosure;
    TH2 *fPearson;
    TH2 *fPearsonClosure;

    bool operator<(const UnfoldingResults &other) const { return fReg < other.fReg; }
    bool operator==(const UnfoldingResults &other) const { return fReg == other.fReg; }
};

struct UnfoldingInput {
    int fReg;
    double fRadius;
    TH1 *fRaw;
    TH1 *fJetFindingEff;
    RooUnfoldResponse *fResponseMatrix;
    TH1 *fDetLevelClosure;
    RooUnfoldResponse *fResponseMatrixClosure;
    TH1 *fJetFindingEffClosure;
};


class UnfoldingHandler {
public:
    enum class UnfoldingMethod_t {
        kBayesian,
        kSVD
    };

    enum class AcceptanceType_t {
        kEMCALFID,
        kTPCFID
    };

    UnfoldingHandler() = default;
    UnfoldingHandler(UnfoldingMethod_t method, AcceptanceType_t atype = AcceptanceType_t::kEMCALFID) : mMethod(method),  mAcceptanceType(atype)  {} 
    ~UnfoldingHandler() = default;

    void setUnfoldingMethod(UnfoldingMethod_t method) { mMethod = method;}

    void setAcceptanceType(AcceptanceType_t atype) { mAcceptanceType = atype; }

    void setErrorTreatment(RooUnfold::ErrorTreatment treatment) { mErrorTreatment = treatment; }

    UnfoldingResults run(UnfoldingInput input) {
        std::string unfoldingtag;
        switch(mMethod) {
            case UnfoldingMethod_t::kBayesian: unfoldingtag = "Bayesian"; break;
            case UnfoldingMethod_t::kSVD: unfoldingtag = "SVD"; break;
        }
        double acceptance = 0.;
        switch(mAcceptanceType) {
            case AcceptanceType_t::kEMCALFID: {
                const double kSizeEmcalPhi = 1.8873487,
                             kSizeEmcalEta = 1.4;
                acceptance = (kSizeEmcalPhi - 2 * input.fRadius) * (kSizeEmcalEta - 2 * input.fRadius) / (TMath::TwoPi());
                break;
            }
            case AcceptanceType_t::kTPCFID: {
                const double kSizeTPCEta = 1.8;
                acceptance = (kSizeTPCEta - 2 * input.fRadius);
                break;
            }

        }
        std::cout << "[" << unfoldingtag << " unfolding] Regularization " << input.fReg << "\n================================================================\n";
        std::cout << "[" << unfoldingtag << " unfolding] Running unfolding" << std::endl;
        auto resultdata = unfold(input.fRaw, input.fResponseMatrix, input.fReg);
        auto specunfolded = resultdata.mSpectrum;
        specunfolded->SetNameTitle(Form("unfolded_reg%d", input.fReg), Form("Unfolded jet spectrum R=%.1f reg %d", input.fRadius, input.fReg));
        specunfolded->SetDirectory(nullptr);
        auto backfolded = MakeRefolded1D(input.fRaw, specunfolded, *input.fResponseMatrix);
        backfolded->SetNameTitle(Form("backfolded_reg%d", input.fReg), Form("back-folded jet spectrum R=%.1f reg %d", input.fRadius, input.fReg));
        backfolded->SetDirectory(nullptr);
        specunfolded->Scale(1., "width");
        auto specnormalizedNoEff = static_cast<TH1 *>(specunfolded->Clone(Form("normalizedNoEffReg%d", input.fReg)));
        specnormalizedNoEff->SetNameTitle(Form("normalizedNoEff_reg%d", input.fReg), Form("Normalized jet spectrum R=%.1f reg %d, no correction for jet finding efficiency", input.fRadius, input.fReg));
        specnormalizedNoEff->SetDirectory(nullptr);
        specnormalizedNoEff->Scale(1. / (acceptance));
        std::cout << "[" << unfoldingtag << " unfolding] Normalizing to acceptance " << acceptance << std::endl;
        auto specnormalized = static_cast<TH1 *>(specnormalizedNoEff->Clone(Form("normalizedReg%d", input.fReg)));
        specnormalized->SetNameTitle(Form("normalized_reg%d", input.fReg), Form("Normalized jet spectrum R=%.1f reg %d", input.fRadius, input.fReg));
        specnormalized->SetDirectory(nullptr);
        specnormalized->Divide(input.fJetFindingEff);
        auto dvec = resultdata.mDvector;
        if(dvec) dvec->SetNameTitle(Form("dvector_Reg%d", input.fReg), Form("D-vector reg %d", input.fReg));

        // run closure test
        std::cout << "[" << unfoldingtag << " unfolding] Running closure test" << std::endl;
        auto resultclosure = unfold(input.fDetLevelClosure, input.fResponseMatrixClosure, input.fReg);
        auto specunfoldedClosureNoEff = resultclosure.mSpectrum;
        specunfoldedClosureNoEff->SetDirectory(nullptr);
        specunfoldedClosureNoEff->SetNameTitle(Form("unfoldedClosureNoEff_reg%d", input.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d, no correction for jet finding efficiency", input.fRadius, input.fReg));
        specunfoldedClosureNoEff->Scale(1., "width");
        auto specunfoldedClosure = static_cast<TH1 *>(specunfoldedClosureNoEff->Clone());
        specunfoldedClosure->SetDirectory(nullptr);
        specunfoldedClosure->SetNameTitle(Form("unfoldedClosure_reg%d", input.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d", input.fRadius, input.fReg));
        if(input.fJetFindingEffClosure)
            specunfoldedClosure->Divide(input.fJetFindingEffClosure);
        else
            specunfoldedClosure->Divide(input.fJetFindingEff);
        TH1 *dvecClosure = resultclosure.mDvector;
        if(dvecClosure) dvecClosure->SetNameTitle(Form("dvectorClosure_Reg%d", input.fReg), Form("D-vector of the closure test reg %d", input.fReg));
        return {input.fReg, specunfolded, specnormalizedNoEff, specnormalized, backfolded, specunfoldedClosureNoEff, specunfoldedClosure, dvec, dvecClosure, 
                CorrelationHist1D(resultdata.mCorrelation, Form("PearsonReg%d", input.fReg), Form("Pearson coefficients regularization %d", input.fReg)),
                CorrelationHist1D(resultclosure.mCorrelation, Form("PearsonClosureReg%d", input.fReg), Form("Pearson coefficients of the closure test regularization %d", input.fReg))};
    } 

private:
    struct UnfoldedData {
        TH1 *mSpectrum = nullptr;
        TMatrixD mCorrelation;
        TH1 *mDvector = nullptr;
    };

    UnfoldedData unfold(TH1 *rawlevel, RooUnfoldResponse *responsematrix, int regularization) {
        switch (mMethod)
        {
        case UnfoldingMethod_t::kSVD: return unfoldSVD(rawlevel, responsematrix, regularization);
        case UnfoldingMethod_t::kBayesian: return unfoldBayes(rawlevel, responsematrix, regularization);
        };
    }

    UnfoldedData unfoldSVD(TH1 *rawlevel, RooUnfoldResponse *responsematrix, int regularization) {
        RooUnfoldSvd unfolder(responsematrix, rawlevel, regularization);
        auto specunfolded = histcopy(unfolder.Hreco(mErrorTreatment));
        specunfolded->SetDirectory(nullptr);
        TH1 *dvec(nullptr);
        auto imp = unfolder.Impl();
        if(imp){
            dvec = histcopy(imp->GetD());
            dvec->SetDirectory(nullptr);
        }
        return {specunfolded, unfolder.Ereco(), dvec};
    }

    UnfoldedData unfoldBayes(TH1 *rawlevel, RooUnfoldResponse *responsematrix, int regularization) {
        RooUnfoldBayes unfolder(responsematrix, rawlevel, regularization);
        auto specunfolded = histcopy(unfolder.Hreco(mErrorTreatment));
        specunfolded->SetDirectory(nullptr);
        return {specunfolded, unfolder.Ereco(), nullptr};
    }

    UnfoldingMethod_t mMethod = UnfoldingMethod_t::kSVD;
    AcceptanceType_t mAcceptanceType = AcceptanceType_t::kEMCALFID;
    RooUnfold::ErrorTreatment mErrorTreatment = RooUnfold::kCovToy;
};

class UnfoldingPool {
public:
    UnfoldingPool() = default;
    ~UnfoldingPool() = default;

    void InsertWork(UnfoldingInput config) {
        std::lock_guard<std::mutex> insert_lock(fAccessLock);
        fData.push(config);
    }

    UnfoldingInput next() {
        std::lock_guard<std::mutex> pop_lock(fAccessLock);
        UnfoldingInput result = fData.front();
        fData.pop();
        return result;
    }

    bool empty() { return fData.empty(); }

private:
    std::mutex                  fAccessLock;
    std::queue<UnfoldingInput>  fData;              
};

class UnfoldingRunner {
    public:
        UnfoldingRunner(UnfoldingPool *work) : mOutputData(), mInputData(work) { }
        ~UnfoldingRunner() = default;

        UnfoldingHandler &getHandler () { return mHandler; }

        void DoWork() {
            while(!mInputData->empty()) {
                mOutputData.push_back(mHandler.run(mInputData->next()));
            }
        }

        const std::vector<UnfoldingResults> &getUnfolded() const { return mOutputData; }

    private:
        std::vector<UnfoldingResults>   mOutputData;       
        UnfoldingPool                   *mInputData;
        UnfoldingHandler                mHandler;
};

#endif