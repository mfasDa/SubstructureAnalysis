#ifndef CLOSURESPECTRAHANDLER_H
#define CLOSURESPECTRAHANDLER_H

#include "../../meta/stl.C"
#include "../../meta/root.C"

#include "MCFileHandler.cxx"

class ClosureSpectraHandler {
public:
    ClosureSpectraHandler(RawResponseMatrix response, const std::vector<double> &partlevelbinning, const std::vector<double> &detlevelbinning):
        fRawResponse(response),
        fPartLevel(nullptr),
        fDetLevel(nullptr),
        fPartLevelBinning(partlevelbinning),
        fDetLevelBinning(detlevelbinning)
    {
        build();
    }

    TH1 *getDetLevelSpectrum() const { return fDetLevel; }
    TH1 *getPartLevelSpectrum() const { return fPartLevel; }

private:
    void build(){
        std::unique_ptr<TH1> projectionPart(fRawResponse.getRawResponse()->ProjectionY(Form("%s_part", fRawResponse.getRawResponse()->GetName()))),
                             projectionDet(fRawResponse.getRawResponse()->ProjectionX(Form("%s_det", fRawResponse.getRawResponse()->GetName())));
        fPartLevel = projectionPart->Rebin(fPartLevelBinning.size()-1, Form("_rebinned", projectionPart->GetName()), fPartLevelBinning.data());
        fDetLevel = projectionDet->Rebin(fDetLevelBinning.size()-1, Form("_rebinned", projectionDet->GetName()), fDetLevelBinning.data());
        fPartLevel->SetDirectory(nullptr);
        fDetLevel->SetDirectory(nullptr);
    }

    RawResponseMatrix fRawResponse;
    TH1 *fPartLevel;
    TH1 *fDetLevel;
    std::vector<double> fPartLevelBinning;
    std::vector<double> fDetLevelBinning;
};
#endif