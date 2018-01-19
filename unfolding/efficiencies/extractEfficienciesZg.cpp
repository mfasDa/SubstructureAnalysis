#ifndef __CLING__
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TGraphErrors.h>
#include <TKey.h>
#include <TList.h>
#endif

struct NBin {
    double min;
    double max;

    double Mean() const { return (max + min)/2.; }
    double Halfwidth() const { return (max - min)/2.; } 

    bool operator<(const NBin &other) const {
        return max <= other.min;
    }

    bool operator==(const NBin &other) const {
        return min == other.min && max == other.max;
    }
};

std::vector<TH1 *> GetListOfEfficiencyHistograms(TFile &reader){
    std::vector<TH1 *> result;
    for(auto k : TRangeDynCast<TKey>(reader.GetListOfKeys())){
        if(TString(k->GetName()).Contains("efficiency")) {
           result.emplace_back(static_cast<TH1 *>(k->ReadObj())); 
        }
    }
    return result;
}

NBin ExtractPtBin(const TH1 *hist) {
    std::string histname = hist->GetName();
    histname.replace(histname.find("efficiency_"), 11, "");
    std::vector<int> bins;
    std::stringstream decoder(histname);
    std::string tmp;
    while(std::getline(decoder, tmp, '_')) bins.emplace_back(std::stoi(tmp));
    return {static_cast<double>(bins[0]), static_cast<double>(bins[1])};
}

std::vector<NBin> GetListOfZgBin(TH1 *hist, double cut = 0){
    std::vector<NBin> result;
    for(auto b : ROOT::TSeqI(1, hist->GetXaxis()->GetNbins()+1)){
        auto min = hist->GetXaxis()->GetBinLowEdge(b),
             max = hist->GetXaxis()->GetBinUpEdge(b);
        if(max < cut) continue;
        result.push_back({min, max});
    }
    std::sort(result.begin(), result.end(), std::less<NBin>());
    return result;
}

void extractEfficienciesZg(std::string_view inputfile){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data()));
    auto effhistos = GetListOfEfficiencyHistograms(*reader);
    std::map<NBin, TGraphErrors *> zgefficienciespt;
    for(auto b : GetListOfZgBin(effhistos[0], 0.1)) {
        auto graph = new TGraphErrors;
        graph->SetName(Form("effzg_%d_%d", int(b.min*100.), int(b.max * 100.)));
        zgefficienciespt[b] = graph;
    }

    for(auto e : effhistos) {
        auto ptbin = ExtractPtBin(e);
        for(auto b : ROOT::TSeqI(1, e->GetXaxis()->GetNbins()+1)){
            auto min = e->GetXaxis()->GetBinLowEdge(b), max = e->GetXaxis()->GetBinUpEdge(b);
            if(max < 0.1) continue;
            auto dist = zgefficienciespt.find({min, max});
            if(dist != zgefficienciespt.end()) {
                auto np = dist->second->GetN();
                dist->second->SetPoint(np, ptbin.Mean(), e->GetBinContent(b));
                dist->second->SetPointError(np, ptbin.Halfwidth(), e->GetBinError(b));
            }
        }
    }

    std::unique_ptr<TFile> writer(TFile::Open("zgefficiencies.root", "RECREATE"));
    for(auto z  : zgefficienciespt) z.second->Write(z.second->GetName());
}