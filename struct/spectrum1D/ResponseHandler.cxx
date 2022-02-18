#ifndef RESPONSEHANDLER_H
#define RESPONSEHANDLER_H

#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/math.C"

#include "MCFileHandler.cxx"

class ResponseHandler {
public:
    ResponseHandler(RawResponseMatrix mat, const std::vector<double> &binningpart, const std::vector<double> binningdet, double scale = 1.) :
        fRawResponse(mat),
        fRebinnedResponse(nullptr),
        fPartLevelBinning(binningpart),
        fDetLevelBinning(binningdet)
    {
        if(TMath::Abs(scale - 1.) > DBL_EPSILON) fRawResponse.Scale(scale);
        build();
    }

    TH2 *getRebinnnedResponse() const { return fRebinnedResponse; }

    TH1 *makeRebinnedThruthSpectrum() const { 
        auto projected = fRebinnedResponse->ProjectionY();
        projected->SetDirectory(nullptr);
        return projected;
    }

    TH1 *makeFullyEfficienctTruthSpectrum() const { 
        std::unique_ptr<TH1> projected(fRawResponse.getRawResponse()->ProjectionY());
        auto rebinned = projected->Rebin(fPartLevelBinning.size()-1, "" , fPartLevelBinning.data());
        rebinned->SetDirectory(nullptr);
        return rebinned;
    }

    TH1 *makeKinematicEfficiency() const {
        auto ratio = makeRebinnedThruthSpectrum();
        std::unique_ptr<TH1> denominator(makeFullyEfficienctTruthSpectrum());
        ratio->Divide(ratio, denominator.get(), 1., 1., "b");
        return ratio;
    }

    TH1 *makeDetLevelSpectrum() const {
        std::unique_ptr<TH1> projected(fRawResponse.getRawResponse()->ProjectionX());
        auto rebinned = projected->Rebin(fDetLevelBinning.size()-1, "" , fDetLevelBinning.data());
        rebinned->SetDirectory(nullptr);
        return rebinned;
    }

private:
    void build() {
        fRebinnedResponse = makeRebinned2D(fRawResponse.getRawResponse(), fDetLevelBinning, fPartLevelBinning);
    }
    RawResponseMatrix fRawResponse;
    TH2 *fRebinnedResponse;
    std::vector<double> fPartLevelBinning;
    std::vector<double> fDetLevelBinning;
};
#endif