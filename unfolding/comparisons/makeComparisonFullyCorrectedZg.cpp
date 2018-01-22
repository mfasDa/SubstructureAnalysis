#ifndef __CLING__
#include <array>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#include <TPaveText.h>
#endif

struct RangedObject {
    double min;
    double max;
    TH1 *hist;

    bool operator==(const RangedObject &other) const {
        return (min == other.min) && (max == other.max);
    }

    bool operator<(const RangedObject &other) const {
        return (max <= other.min);
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

std::array<int, 2> GetPtRange(std::string_view rangestring){
    std::istringstream parser((std::string)rangestring);
    std::vector<std::string> tokens;
    std::string tmp;
    while(std::getline(parser, tmp, '_')) tokens.push_back(tmp);
    return std::array<int, 2> {{std::stoi(tokens[2]), std::stoi(tokens[3])}};
}

void makePlotTrigger(std::string_view filename, std::string_view trigger, double radius){
    std::set<RangedObject> histograms;
    {
        auto filereader = make_unique<TFile>(TFile::Open(filename.data(), "READ"));
        for(auto h : TRangeDynCast<TKey>(filereader->GetListOfKeys())){
            std::string histname = h->GetName();
            if(histname.find("corrected_zg_") != std::string::npos){
                auto hist = h->ReadObject<TH1>();
                hist->SetDirectory(nullptr);
                auto range = GetPtRange(histname);
                histograms.insert({range[0], range[1], hist});
            }
        }
    }

    auto axis = new TH1F(Form("zgaxis_%s", trigger.data()), "; z_{g}; 1/N_{jet} dN/dz_{g}", 100, 0., 0.6);
    axis->SetStats(false);
    axis->GetYaxis()->SetRangeUser(0., 0.3);
    axis->Draw("axis");

    auto leg = new TLegend(0.65, 0.5, 0.89, 0.89);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->Draw();

    auto label = new TPaveText(0.15, 0.15, 0.45, 0.2, "NDC");
    label->SetBorderSize(0);
    label->SetFillStyle(0);
    label->SetTextFont(42);
    label->AddText(Form("Trigger: %s,  R=%0.1f", trigger.data(), radius));
    label->Draw();

    int histcounter(0);
    const std::array<Color_t, 10> COLORS  = {{kBlack, kRed, kBlue, kGreen, kOrange, kViolet, kCyan, kMagenta, kGray, kAzure}};
    const std::array<Style_t, 10> MARKERS = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};
    for(auto h : histograms) {
        h.hist->SetMarkerColor(COLORS[histcounter]);
        h.hist->SetLineColor(COLORS[histcounter]);
        h.hist->SetMarkerStyle(MARKERS[histcounter]);
        h.hist->Draw("epsame");
        leg->AddEntry(h.hist, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", h.min, h.max), "lep");
    }

    gPad->cd();
    gPad->Update();
}

void makeComparisonFullyCorrectedZg(){
    std::array<double, 2> jetradii = {{0.2, 0.4}};
    std::array<std::string, 3> triggers = {{"INT7", "EJ1", "EJ2"}};

    auto plot = new TCanvas("comparisonFullyCorrected", "Fully corrected zg", 1200, 800);
    plot->Divide(3,2);
    int irad(0), itrg(0);
    for(auto radius : jetradii){
        for(auto trg : triggers){
            plot->cd(irad*3+itrg+1);
            makePlotTrigger(Form("Unfolded_Zg_R%02d_%s/corrected_zg_R%02d_%s.root", int(radius * 10.), trg.data(), int(radius * 10.), trg.data()), trg, radius);
            itrg++;
        }
        irad++;
    }
}