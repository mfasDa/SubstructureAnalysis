#ifndef __CLING__
#include <memory>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TPaveText.h>
#endif

struct PStyle{
    Color_t         fColor;
    Style_t         fMarker;
};

TH1 *readFile(std::string_view filename, std::string_view title, const PStyle &style){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto mean = static_cast<TH1 *>(reader->Get("ptdiffmean"));
    mean->SetDirectory(nullptr);
    mean->SetTitle(title.data());
    mean->SetMarkerColor(style.fColor);
    mean->SetLineColor(style.fColor);
    mean->SetMarkerStyle(style.fMarker);
    return mean;
}

void makeEnergyComparisonPtShiftFull(){
    auto plot = new TCanvas("comparisonPlot", "Energy comparison", 800, 600);
    plot->cd();

    auto axis = new TH1F("axis", "; p_{t, part} (GeV/c); (p_{t, det} - p_{t,part}) / p_{t, part}", 200, 0., 200.);
    axis->SetDirectory(nullptr);
    axis->SetStats(false);
    axis->GetYaxis()->SetRangeUser(-0.4, 0.4);
    axis->Draw("axis");

    auto leg = new TLegend(0.5, 0.7, 0.89, 0.89);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->Draw();

    auto label = new TPaveText(0.15, 0.15, 0.45, 0.22, "NDC");
    label->SetBorderSize(0);
    label->SetFillStyle(0);
    label->SetTextFont(42);
    label->AddText("full jets, anti-k_{t}, R=0.4");
    label->Draw();

    auto h276 = readFile("LHC12a15a/EnergyScale.root", "LHC12a15a, pp #sqrt{s} = 2.76 TeV", {kBlack, 24});
    h276->Draw("epsame");
    leg->AddEntry(h276, h276->GetTitle(), "lep");

    auto h13 = readFile("LHC17f8a/EnergyScale.root", "LHC17f8a, pp #sqrt{s} = 13 TeV", {kRed, 25});
    h13->Draw("epsame");
    leg->AddEntry(h13, h13->GetTitle(), "lep");

    plot->Update();
}