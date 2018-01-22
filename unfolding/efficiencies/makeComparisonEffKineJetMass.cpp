#ifndef __CLING__
#include <array>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <RStringView.h>

#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#include <TObjArray.h>
#endif

struct RangedObject {
    double min;
    double max;
    TH1 *hist;

    bool operator==(const RangedObject &other) const {
        return (min == other.min) && (max == other.max);
    }

    bool operator<(const RangedObject &other) const {
        return max <= other.min;
    }

    std::string rangestring(){
        std::stringstream rangehandler;
        rangehandler.setf(ios::fixed,ios::floatfield);
        rangehandler << std::setprecision(1) << min << " GeV/c < p_{t} < " << max << " GeV/c";
        return rangehandler.str();
    }
};

template<typename t>
t *Get(TFile *reader, const char *key){
    t *result(nullptr);
    result = static_cast<t *>(reader->Get(key));
    return result;
}

template<typename t>
std::unique_ptr<t> make_unique(t *object){
    std::unique_ptr<t> ptr(object);
    return ptr;
}

std::array<int, 2> GetPtRange(std::string_view histname){
    const auto TAG = "efficiency_";
    std::array<int,2> range;
    auto ptstring = histname.substr(histname.find(TAG) + strlen(TAG), histname.size() - strlen(TAG));
    auto delim = ptstring.find("_");
    range[0] = std::stoi((std::string)ptstring.substr(0, delim));
    range[1] = std::stoi((std::string)ptstring.substr(delim+1, ptstring.length() - delim - 1));
    return range;
}

void makeComparisonEffKineJetMass(std::string_view filename){
    const std::array<Color_t, 10> COLORS = {{kBlack, kBlue, kRed, kGreen, kOrange, kMagenta, kCyan, kGray, kViolet, kAzure}};
    const std::array<Style_t, 10> MARKERS = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};
    auto contains = [](std::string_view source, std::string_view text) { return source.find(text) != std::string::npos; };
    std::set<RangedObject> efficiencies;
    auto filereader = make_unique<TFile>(TFile::Open(filename.data(), "READ"));
    for(auto k : *(filereader->GetListOfKeys())){
        if(!contains(k->GetName(), "efficiency")) continue;
        auto ptrange = GetPtRange(k->GetName());
        if((double)ptrange[0] < 20.) continue;
        auto hist = static_cast<TH1 *>(static_cast<TKey *>(k)->ReadObj());
        hist->SetDirectory(nullptr);
        hist->SetStats(false);
        efficiencies.insert({(double)ptrange[0], (double)ptrange[1], hist});
    }

    auto plot = new TCanvas("plotEffKineMass", "Kinematic efficiency jet mass", 800, 600);
    plot->cd();

    auto axis = new TH1F("axis", "; m (GeV/c^{2}); kin. eff.", 100, 0., 40.);
    axis->SetDirectory(nullptr);
    axis->SetStats(false);
    axis->GetYaxis()->SetRangeUser(0., 1.3);
    axis->Draw("axis");

    auto leg = new TLegend(0.15, 0.75, 0.89, 0.89);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->SetNColumns(2);
    leg->Draw();
    
    int nhist(0);
    for(auto h : efficiencies) {
        h.hist->SetMarkerColor(COLORS[nhist]);
        h.hist->SetLineColor(COLORS[nhist]);
        h.hist->SetMarkerStyle(MARKERS[nhist]);
        h.hist->Draw("epsame");
        leg->AddEntry(h.hist, h.rangestring().data(), "lep");
        nhist++;
    }
    plot->cd();
    plot->Update();
}