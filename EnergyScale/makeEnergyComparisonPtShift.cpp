#ifndef __CLING__
#include <memory>
#include <vector>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TProfile.h>
#endif

struct style {
    Color_t color;
    Style_t marker;
};

TH1 *ReadPtShift(std::string_view filename, std::string_view title, style mystyle){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto result = static_cast<TProfile *>(reader->Get("ptdiffmean"));
    result->SetDirectory(nullptr);
    result->SetTitle(title.data());
    result->SetMarkerColor(mystyle.color);
    result->SetLineColor(mystyle.color);
    result->SetMarkerStyle(mystyle.marker);
    return result;
}

void makeEnergyComparisonPtShift(){
    std::vector<TH1 *> ptdiffs = {
        ReadPtShift("1377-charge7TeV-fixeff/EnergyScale_R02.root", "pp, #sqrt{s} = 7 TeV, R=0.2", {kBlack, 24}),
        ReadPtShift("1377-charge7TeV-fixeff/EnergyScale_R04.root", "pp, #sqrt{s} = 7 TeV, R=0.4", {kBlack, 25}),
        ReadPtShift("1382-chargedJet5TeV/EnergyScale_R02.root", "pp, #sqrt{s} = 5.02 TeV, R=0.2", {kBlue, 24}),
        ReadPtShift("1382-chargedJet5TeV/EnergyScale_R04.root", "pp, #sqrt{s} = 5.02 TeV, R=0.4", {kBlue, 25}),
        ReadPtShift("1379-charged13TeV-fixeff/EnergyScale_R02.root", "pp, #sqrt{s} = 13 TeV, R=0.2", {kRed, 24}),
        ReadPtShift("1379-charged13TeV-fixeff/EnergyScale_R04.root", "pp, #sqrt{s} = 13 TeV, R=0.4", {kRed, 25})
    };

    auto plot = new TCanvas("energyComparison", "Comparison pt-shift energies", 800, 600);

    auto frame = new TH1F("ptshiftframe", ";p_{t, part} (GeV/c); <(p_{t,det} - p_{t,part})/p_{t,part}>", 200, 0., 200);
    frame->SetDirectory(nullptr);
    frame->SetStats(false);
    frame->GetYaxis()->SetRangeUser(-0.4, 0.4);
    frame->Draw("axis");

    auto leg = new TLegend(0.6, 0.65, 0.89, 0.89);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->Draw();

    auto label = new TPaveText(0.15, 0.15, 0.45, 0.22, "NDC");
    label->SetBorderSize(0);
    label->SetFillStyle(0);
    label->SetTextFont(42);
    label->AddText("pp, #sqrt{s} = 7 TeV, anti-kt");
    label->Draw();

    for(auto testcase : ptdiffs){
        testcase->Draw("epsame");
        leg->AddEntry(testcase, testcase->GetTitle(), "lep");
    }
    plot->Update();
}