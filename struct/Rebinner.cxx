#ifndef __REBINNER_C__
#define __REBINNER_C__

#include "../meta/stl.C"
#include "../meta/root.C"

class Rebinner {
public:
    Rebinner(const std::vector<double> &binning) : mBinning(binning) {}
    ~Rebinner() = default;

    TH1 *operator()(TH1 *toRebin) { 
        auto result = toRebin->Rebin(mBinning.size() - 1, Form("%s_rebinned", toRebin->GetName()), mBinning.data()); 
        result->Scale(1., "width");
        return result;
    }

private:
    std::vector<double>         mBinning;
};

#endif // __REBINNER_C__