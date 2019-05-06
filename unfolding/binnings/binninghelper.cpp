#ifndef __BINNINGHELPER_C__
#define __BINNINGHELPER_C__

#include "../../meta/stl.C"

struct binningstep {
    double fMaximum;
    double fBinsize;
};

class binninghelper {
public:
    binninghelper() = default;
    binninghelper(double minimum, std::vector<binningstep> steps) : fMinimum(minimum), fSteps(steps) {}
    ~binninghelper() = default;

    void SetMinimum(double minimum) { fMinimum = minimum; } 
    void AddStep(double maximum, double stepsize) { fSteps.push_back({maximum, stepsize}); }
    std::vector<double> CreateCombinedBinning() const {
        std::vector<double> binning;
        double current = fMinimum;
        binning.emplace_back(current);
        for(const auto [max, stepsize] : fSteps ) {
            while(current < max) {
                current += stepsize;
                binning.emplace_back(current);
            }   
        }
        return binning;
    }
public:
    double                      fMinimum;
    std::vector<binningstep>    fSteps;
};
#endif //__BINNINGHELPER_C__