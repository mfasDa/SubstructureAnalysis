#ifndef __CLING__
#include <memory>
#include <array>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TObjArray.h>
#include <TObjString.h>
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

std::array<int, 2> GetPtRange(TString rangestring){
    auto tokens = make_unique<TObjArray>(rangestring.Tokenize("_"));
    return std::array<int, 2> {{static_cast<TObjString *>(tokens->At(0))->String().Atoi(), static_cast<TObjString *>(tokens->At(0))->String().Atoi()}};
}
void makePlotTrigger(std::string_view filename, std::string_view trigger){
    auto filereader = make_unique<TFile>(TFile::Open(filename.data(), "READ"));

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
    label->AddText(Form("Trigger: %s", trigger.data()));
    label->Draw();

    const std::array<Color_t, 10> COLORS  = {{kBlack, kRed, kBlue, kGreen, kOrange, kViolet, kCyan, kMagenta, kGray, kAzure}};
    const std::array<Style_t, 10> MARKERS = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};

    int nspec = 0;
    for(auto h : *reader->GetListOfKeys()){
        TString histname = h->GetName();
        if(histname.Contains("corrected_zg_"));
    }

    gPad->cd();
    gPad->Update();
}

void makeComparisonFullyCorrectedZg(){

}