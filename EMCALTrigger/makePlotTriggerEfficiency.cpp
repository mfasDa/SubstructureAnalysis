#ifndef __CLING__
#include <map>
#include <memory>
#include <string>

#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#endif

void makePlotTriggerEfficiency(std::string_view filename) {
    auto plot = new TCanvas("triggerEfficiencyPlot", "Trigger efficiency", 800, 600);
    plot->cd();
    auto axis = new TH1F("effaxis", "; p_{t,jet} (GeV/c); Trigger efficiency", 200, 0., 200.);
    axis->SetDirectory(nullptr);
    axis->SetStats(false);
    axis->GetYaxis()->SetRangeUser(0., 1.1);
    axis->Draw("axis");

    auto leg = new TLegend(0.5, 0.15, 0.89, 0.35);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->Draw();

    std::map<std::string, Color_t> colors = {{"EJ1", kRed}, {"EJ2", kBlue}};
    std::map<std::string, Style_t> markers = {{"EJ1", 24}, {"EJ2", 25}};

    auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
    reader->cd("efficiencies");
    for(auto eff : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
        auto hist = eff->ReadObject<TH1>();
        std::string triggerclass = hist->GetName();
        triggerclass.replace(triggerclass.find("efficiency"), 10, "");
        Color_t color =colors.find(triggerclass)->second;
        hist->SetDirectory(nullptr);
        hist->SetStats(false);
        hist->SetMarkerColor(color);
        hist->SetLineColor(color);
        hist->SetMarkerStyle(markers.find(triggerclass)->second);
        hist->Draw("epsame");
        leg->AddEntry(hist, triggerclass.data(), "lep");
    }
    plot->Update();
}