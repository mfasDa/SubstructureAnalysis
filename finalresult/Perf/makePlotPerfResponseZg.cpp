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
    auto plot = new ROOT6tools::TSavableCanvas(Form("PerfResponsematrixzg_R%02d_%d_%d",radius, int(ptmin), int(ptmax)), Form("Response matrix, jet radius R=%.1f", double(radius)/10.), 1000, 800);
    plot->SetLogz();
    plot->SetRightMargin(0.14);
    plot->SetLeftMargin(0.13);
    plot->SetTopMargin(0.04);
    auto frame = new ROOT6tools::TAxisFrame("zgframe", "z_{g}^{det}", "z_{g}^{part}", 0., 0.55, 0., 0.8);
    double axissize = 0.045;
    frame->GetXaxis()->SetTitleSize(axissize);
    frame->GetYaxis()->SetTitleSize(axissize);
    frame->GetXaxis()->SetLabelSize(axissize);
    frame->GetYaxis()->SetLabelSize(axissize);
    frame->Draw("axis");
    responsematrix->GetZaxis()->SetRangeUser(1e-10, 1e-4);
    responsematrix->SetDirectory(nullptr);
    responsematrix->Draw("colzsame");
    auto label = new ROOT6tools::TNDCLabel(0.13, 0.65, 0.75, 0.96, "ALICE Simulation");
    label->AddText("pp #sqrt{s} = 13 TeV, PYTHIA8 Monash(2013)");
    label->AddText(Form("Anti-k_{t}, #it{R} = %.1f, %.1f GeV/#it{c} < #it{p}_{T,jet} < %.1f GeV/#it{c}", double(radius)/10., ptmin, ptmax));
    label->AddText("#it{p}_{T}^{track} > 0.15 GeV/#it{c}, #it{E}^{cluster} > 0.3 GeV");
    label->AddText(Form("|#it{#eta}^{track,cluster}| < 0.7, |#it{#eta}^{jet}| < %.1f", 0.7 - double(radius)/10.));
    label->AddText("SoftDrop: z_{cut} = 0.1, #it{#beta} = 0");
    label->SetTextAlign(12);
    label->SetTextSize(0.045);
    label->Draw();

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}