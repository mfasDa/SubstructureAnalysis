#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

TH1 *readEfficiency(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto hist = static_cast<TH1 *>(reader->Get("efficiency"));
    hist->SetDirectory(nullptr);
    return hist;
}

void compareTrackingEffMinBiasYears() {
    std::map<int, Style> configs = {{2016, {kRed, 24}}, {2017, {kBlue, 25}}, {2018, {kGreen, 26}}};
    auto plot = new ROOT6tools::TSavableCanvas("ComparisonTrackingEffYears", "Comparison tracking efficiency years", 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("effframe", "p_{t} (GeV/c)", "Tracking efficiency", 0., 100., 0., 1.1))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.75, 0.89, "hybrid tracks, full acceptance"));
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.4);
    leg->Draw();
    for(auto c : configs) {
        auto spec = readEfficiency(Form("%d_MB/trackingEfficiency_full.root", c.first));
        c.second.SetStyle<TH1>(*spec);
        spec->Draw("epsame");
        leg->AddEntry(spec, Form("%d", c.first), "lep");
    }
    plot->cd();
    plot->Draw();
    plot->SaveCanvas(plot->GetName());
}