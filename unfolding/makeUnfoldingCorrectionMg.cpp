
#ifndef __CLING__
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>
#endif

struct PtBin {
    double               fPtMin;
    double               fPtMax;
    TH1*                 fData;

    bool operator<(const PtBin &other) const { return fPtMax <= other.fPtMin; }
    bool operator==(const PtBin &other) const { return fPtMin == other.fPtMin && fPtMax == other.fPtMax; }
};

bool contains(std::string_view str, std::string_view text){
    return str.find(text) != std::string::npos;
}

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

std::vector<PtBin> ExtractEfficiencies(TFile *data){
    const char *TAG = "efficiency_";
    std::vector<PtBin> result;
    for(auto k : TRangeDynCast<TKey>(data->GetListOfKeys())){
        if(!contains(k->GetName(), "efficiency")) continue;
        auto hist = static_cast<TKey *>(k)->ReadObject<TH1>();
        hist->SetDirectory(nullptr);
        auto ptstring = std::string_view(hist->GetName()).substr(strlen(TAG), strlen(hist->GetName()) - strlen(TAG));
        std::cout << ptstring << std::endl;
        auto delim = ptstring.find("_");
        auto ptmin = std::stoi(std::string(ptstring.substr(0, delim)));
        auto ptmax = std::stoi(std::string(ptstring.substr(delim+1, ptstring.length()-(delim+1))));
        result.push_back({(double)ptmin, (double)ptmax, hist});
    }
    std::sort(result.begin(), result.end(), std::less<PtBin>());
    return result;
}

std::vector<PtBin> ExtractUnfoldedBins(const TH2 *data) {
    std::vector<PtBin> result;
    for(auto b : ROOT::TSeqI(1, data->GetYaxis()->GetNbins()+1)){
        auto slice = data->ProjectionX("slice", b, b);
        slice->SetDirectory(nullptr);
        slice->SetName(Form("mg_slice_%d_%d", int(data->GetYaxis()->GetBinLowEdge(b)), int(data->GetYaxis()->GetBinUpEdge(b))));
        result.push_back({data->GetYaxis()->GetBinLowEdge(b), data->GetYaxis()->GetBinUpEdge(b), slice});
    }
    std::sort(result.begin(), result.end(), std::less<PtBin>());
    return result;
}

std::string ExtractTrigger(std::string_view unfoldedfile){
    std::string result;
    if(contains(unfoldedfile, "EJ1")) result = "EJ1";
    else if(contains(unfoldedfile, "EJ2")) result = "EJ2";
    else if(contains(unfoldedfile, "INT7")) result = "INT7";
    return result;
}

double ExtractRadius(std::string_view unfoldedfile){
    double radius(0.);
    std::istringstream parser((std::string)unfoldedfile);
    std::string tmp;
    while(std::getline(parser, tmp, '_')){
        if(tmp.find("R") != std::string::npos){
            radius = double(std::stoi(tmp.substr(1,2)))/10.;
            break;
        }
    }
    return radius;
}

void makeUnfoldingCorrectionMg(std::string_view unfoldedfile, int iteration = 4){
    // Steps:
    // 1. Correction for undfolding (done in previous step)
    // 2. Correction for efficiency (here)
    // Use spectrum at iteration 4
    auto filereader = make_unique<TFile>(TFile::Open(unfoldedfile.data()));
    auto unfolded = make_unique<TH2>(Get<TH2>(filereader.get(), Form("mass_unfolded_iter%d", iteration)));   
    auto efficiencies = ExtractEfficiencies(filereader.get());
    auto unfoldedbins = ExtractUnfoldedBins(unfolded.get());

    std::vector<TH1 *> corrected;
    for(auto s : unfoldedbins){ 
        auto effbin = std::find(efficiencies.begin(), efficiencies.end(), s);
        if(effbin == efficiencies.end()) {
            std::cerr << "No efficiency found for " << s.fPtMin << " < pt < " << s.fPtMax << std::endl;
            continue;
        }
        auto correctedbin = static_cast<TH1 *>(s.fData->Clone(Form("corrected_mg_%d_%d", (int)s.fPtMin, (int)s.fPtMax)));
        correctedbin->SetDirectory(nullptr);
        correctedbin->Divide(effbin->fData);
        // Normalize
        // setting bin 0 to 0
        correctedbin->Scale(1./correctedbin->Integral());
        corrected.emplace_back(correctedbin);
    }

    auto trigger = ExtractTrigger(unfoldedfile);
    auto radius = ExtractRadius(unfoldedfile);
    std::stringstream outname;
    outname <<  "corrected_mg_R" << std::setw(2) << std::setfill('0') << int(radius * 10.) << "_" << trigger << ".root";
    auto outputwriter = make_unique<TFile>(TFile::Open(outname.str().data(), "RECREATE"));
    for(auto c : corrected) c->Write();
}
