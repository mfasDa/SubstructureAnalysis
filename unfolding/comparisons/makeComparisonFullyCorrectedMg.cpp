
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

#include "../../helpers/graphics.C"

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


std::array<double, 2> GetRangeForTrigger(std::string_view triggerstring) {
    std::array<double, 2> result;
    if(triggerstring.find("INT7") != std::string::npos){
        result[0] = 40.;
        result[1] = 80.;
    } else if(triggerstring.find("EJ1") != std::string::npos) {
        result[0] = 140.;
        result[1] = 200.;
    } else if(triggerstring.find("EJ2") != std::string::npos) {
        result[0] = 80.;
        result[1] = 140.;
    }
    return result;
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
            if(histname.find("corrected_mg_") != std::string::npos){
                auto hist = h->ReadObject<TH1>();
                hist->SetDirectory(nullptr);
                hist->Scale(1./hist->Integral());
                auto range = GetPtRange(histname);
                histograms.insert({static_cast<double>(range[0]), static_cast<double>(range[1]), hist});
            }
        }
    }

    auto select_range = GetRangeForTrigger(trigger);

    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.06);

    auto axis = new TH1F(Form("massaxis_%s_R%02d", trigger.data(), int(radius * 10.)), "; M_{g} (GeV/c^{2}); 1/N_{jet} dN/dM_{g} ((GeV/c^{2})^{-1})", 100, 0., 40.);
    axis->SetStats(false);
    axis->GetYaxis()->SetRangeUser(0., 0.2);
    axis->Draw("axis");

    auto leg = new TLegend(0.51, 0.5, 0.91, 0.89);
    InitWidget<TLegend>(*leg);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->Draw();

    auto label = new TPaveText(0.15, 0.8, 0.45, 0.85, "NDC");
    InitWidget<TPaveText>(*label);
    label->AddText(Form("Trigger: %s,  R=%0.1f", trigger.data(), radius));
    label->Draw();

    int histcounter(0);
    const std::array<Color_t, 10> COLORS  = {{kBlack, kRed, kBlue, kGreen, kOrange, kViolet, kCyan, kMagenta, kGray, kAzure}};
    const std::array<Style_t, 10> MARKERS = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};
    for(auto h : histograms) {
        if(h.max <= select_range[0] || h.min >= select_range[1]) continue;
        Style mystyle{COLORS[histcounter], MARKERS[histcounter]};
        mystyle.SetStyle<TH1>(*(h.hist));
        h.hist->Draw("epsame");
        leg->AddEntry(h.hist, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", h.min, h.max), "lep");
        histcounter++;
    }

    gPad->cd();
    gPad->Update();
}

void makeComparisonFullyCorrectedMg(){
    std::array<double, 2> jetradii = {{0.2, 0.4}};
    std::array<std::string, 3> triggers = {{"INT7", "EJ1", "EJ2"}};

    auto plot = new TCanvas("comparisonFullyCorrected", "Fully corrected mass", 1200, 800);
    plot->Divide(3,2);
    int irad(0), itrg(0);
    for(auto radius : jetradii){
        itrg=0;
        for(auto trg : triggers){
            plot->cd(irad*3+itrg+1);
            makePlotTrigger(Form("Unfolded_Mg_R%02d_%s/corrected_mg_R%02d_%s.root", int(radius * 10.), trg.data(), int(radius * 10.), trg.data()), trg, radius);
            itrg++;
        }
        irad++;
    }
}