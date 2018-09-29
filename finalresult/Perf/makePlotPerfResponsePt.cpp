#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/string.C"

int extractR(const std::string_view filename){
    auto tokens = tokenize(std::string(filename), '_');
    return std::stoi(tokens[2].substr(1));
}

void makePlotPerfResponsePt(const std::string_view filename){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto responsematrix = static_cast<TH2 *>(reader->Get("matrixfine_allptsim"));

    auto radius = extractR(filename);
    auto plot = new ROOT6tools::TSavableCanvas(Form("PerfResponsematrixpt_R%02d",radius), Form("Response matrix, jet radius R=%.1f", double(radius)/10.), 800, 600);
    plot->SetLogz();
    plot->SetRightMargin(0.14);
    auto frame = new ROOT6tools::TAxisFrame("respframe", "p_{t}^{jet,particle} (GeV/c)", "p_{t}^{jet,detector} (GeV/c)", 20., 200., 10., 300.);
    frame->GetXaxis()->SetTitleOffset(1.2);
    frame->Draw("axis");
    responsematrix->GetZaxis()->SetRangeUser(1e-10, 1);
    responsematrix->SetDirectory(nullptr);
    responsematrix->Draw("colzsame");
    auto label = new ROOT6tools::TNDCLabel(0.15, 0.7, 0.75, 0.89, "ALICE simulation, PYTHIA8 Monash(2013), pp #sqrt{s} = 13 TeV");
    label->AddText(Form("Jets, FastJet anti-k_{t}, R=%.1f", double(radius)/10.));
    label->AddText("p_{t}^{track} > 0.15 GeV/c, E^{cluster} > 0.3 GeV");
    label->AddText(Form("|#eta^{track,cluster}| < 0.7, |#eta^{jet}| < %.1f", 0.7-double(radius)/10.));
    label->SetTextAlign(12);
    label->Draw();

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}