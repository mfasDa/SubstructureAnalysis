#ifndef RESTRICTOR_H
#define RESTRICTOR_H

#include "../meta/stl.C"
#include "../meta/root.C"

class Restrictor {
public:
    Restrictor(double ptmin, double ptmax) : mptmin(ptmin), mptmax(ptmax) {}
    TH1 *operator()(const TH1 *inputspectrum) const {
        std::vector<double> binlimits;
        for(int ib : ROOT::TSeqI(0, inputspectrum->GetXaxis()->GetNbins())){
            if(ib == 0) {
                auto binmin = inputspectrum->GetXaxis()->GetBinLowEdge(1);
                if(binmin >= mptmin && binmin <= mptmax) binlimits.push_back(binmin);
            }
            auto binmax = inputspectrum->GetXaxis()->GetBinUpEdge(ib+1);
            if(binmax >= mptmin && binmax <= mptmax) binlimits.push_back(binmax);
        }
        TH1 *restricted = new TH1D(Form("%s_restricted", inputspectrum->GetName()), inputspectrum->GetTitle(), binlimits.size()-1, binlimits.data());
        restricted->SetDirectory(nullptr);
        for(auto ib : ROOT::TSeqI(0, restricted->GetXaxis()->GetNbins())){
            auto binorig = inputspectrum->GetXaxis()->FindBin(restricted->GetXaxis()->GetBinCenter(ib+1));
            restricted->SetBinContent(ib+1, inputspectrum->GetBinContent(binorig));
            restricted->SetBinError(ib+1, inputspectrum->GetBinError(binorig));
        }
        return restricted;
    }
private:
    double mptmin, mptmax;
};

#endif