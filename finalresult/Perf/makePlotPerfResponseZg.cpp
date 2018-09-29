#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/string.C"

int extractR(const std::string_view filename){
    auto tokens = tokenize(std::string(filename), '_');
    return std::stoi(tokens[2].substr(1));
}

void makePlotPerfResponseZg(const std::string_view filename, double ptmin, double ptmax){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto responsematrix = static_cast<TH2 *>(reader->Get(Form("matrixfine_%d_%d", int(ptmin), int(ptmax))));

    auto radius = extractR(filename);
    auto plot = new ROOT6tools::TSavableCanvas(Form("PerfResponsematrixzg_R%02d_%d_%d",radius, int(ptmin), int(ptmax)), Form("Response matrix, jet radius R=%.1f", double(radius)/10.), 800, 600);
    plot->SetLogz();
    plot->SetRightMargin(0.14);
    auto frame = new ROOT6tools::TAxisFrame("zgframe", "z_{g}^{jet,deector}", "z_{g}^{particle}", 0., 0.55, 0., 0.7);
    frame->Draw("axis");
    responsematrix->GetZaxis()->SetRangeUser(1e-10, 1e-4);
    responsematrix->SetDirectory(nullptr);
    responsematrix->Draw("colzsame");
    auto label = new ROOT6tools::TNDCLabel(0.15, 0.7, 0.75, 0.89, "ALICE simulation, PYTHIA8 Monash(2013), pp #sqrt{s} = 13 TeV");
    label->AddText(Form("Jets, FastJet anti-k_{t}, R=%.1f", double(radius)/10.));
    label->AddText(Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptmin, ptmax));
    label->AddText("p_{t}^{track} > 0.15 GeV/c, E^{cluster} > 0.3 GeV");
    label->AddText(Form("|#eta^{track,cluster}| < 0.7, |#eta^{jet}| < %.1f", 0.7 - double(radius)/10.));
    label->SetTextAlign(12);
    label->Draw();

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}