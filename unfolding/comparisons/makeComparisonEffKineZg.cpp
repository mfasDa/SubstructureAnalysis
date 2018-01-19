#ifndef __CLING__
#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
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
    range[0] = std::stoi(ptstring.substr(0, delim));
    range[1] = std::stoi(ptstring.substr(delim+1, ptstring.length() - delim - 1));
    return range;
}

void makeComparisonEffKineZg(std::string_view filename){
    const std::array<Color_t, 10> COLORS = {{kBlack, kBlue, kRed, kGreen, kOrange, kMagenta, kCyan, kGray, kViolet, kAzure}};
    const std::array<Style_t, 10> MARKERS = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};
    auto contains = [](std::string_view source, std::string_view text) { return source.find(text) != std::string::npos; };
    std::vector<TH1 *> efficiencies;
    auto filereader = make_unique<TFile>(TFile::Open(filename.data(), "READ"));
    for(auto k : *(filereader->GetListOfKeys())){
        if(!contains(k->GetName(), "efficiency")) continue;
        auto hist = static_cast<TH1 *>(static_cast<TKey *>(k)->ReadObj());
        hist->SetDirectory(nullptr);
        hist->SetStats(false);
        auto ptrange = GetPtRange(hist->GetName());
        hist->SetTitle(Form("%.01f GeV/c < p_{t, part} < %.01f GeV/c", (double)ptrange[0], (double)ptrange[1]));
        efficiencies.emplace_back(hist);
    }

    auto plot = new TCanvas("plotEffKineZg", "Kinematic efficiency zg", 800, 600);
    plot->cd();

    auto axis = new TH1F("axis", "; z_{g}; kin. eff.", 100, 0., 0.6);
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
        h->SetMarkerColor(COLORS[nhist]);
        h->SetLineColor(COLORS[nhist]);
        h->SetMarkerStyle(MARKERS[nhist]);
        h->Draw("epsame");
        leg->AddEntry(h, h->GetTitle(), "lep");
        nhist++;
    }
    plot->cd();
    plot->Update();
}