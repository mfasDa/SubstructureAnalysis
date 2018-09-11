#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

TH1 *readEffKine(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("detectorresponse");
    auto hist = static_cast<TH1 *>(gDirectory->Get("effKine"));
    hist->SetDirectory(nullptr);
    return hist;
}

void makePlotKinematicEfficiency(){
    auto plot = new ROOT6tools::TSavableCanvas("effKineSpecPt", "Kinematic efficiency comparison", 800, 600);
    (new ROOT6tools::TAxisFrame("compframe", "p_{t} (GeV/c)", "Kinematic efficiency", 0., 250., 0., 1.))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.45);
    leg->Draw();

    std::map<int, Style> styles = {{2, {kRed, 24}}, {3, {kGreen, 25}}, {4, {kBlue, 26}}, {5, {kViolet, 27}}};

    for(auto radius : ROOT::TSeqI(2, 6)){
        auto eff = readEffKine(Form("corrected1DBayes_R%02d.root", radius));
        eff->SetName(Form("efficiencyR%02d", radius));
        styles[radius].SetStyle<TH1>(*eff);
        eff->Draw("epsame");
        leg->AddEntry(eff, Form("R=%0.1f", float(radius)/10.), "lep");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}