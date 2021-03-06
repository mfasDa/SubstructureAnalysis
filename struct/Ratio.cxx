#ifndef __RATIO_C__
#define __RATIO_C__

#include "../meta/root.C"

class Ratio : public TH1D{
public:
    Ratio() = default;
    Ratio(const TH1 *numerator, const TH1 *denominator, Option_t *option = "") : TH1D(*static_cast<const TH1D *>(numerator)) {
        SetName(Form("ratio_%s_%s", numerator->GetName(), denominator->GetName())); 
        if(strlen(option)) Divide(numerator, denominator, 1., 1., option);
        else Divide(denominator);
        SetDirectory(nullptr);
    }
    virtual ~Ratio() = default;

    TH1D *makeTH1(const char *name, const char *title){
        TH1D *result = new TH1D(*static_cast<TH1D*>(this));
        result->SetNameTitle(name, title);
        return result;
    }
};
#endif