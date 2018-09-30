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
    auto plot = new ROOT6tools::TSavableCanvas(Form("PerfResponsematrixpt_R%02d",radius), Form("Response matrix, jet radius R=%.1f", double(radius)/10.), 1000, 800);
    plot->SetLogz();
    plot->SetLeftMargin(0.14);
    plot->SetBottomMargin(0.14);
    plot->SetTopMargin(0.04);
    plot->SetRightMargin(0.14);
    auto frame = new ROOT6tools::TAxisFrame("respframe", "#it{p}_{T,jet}^{part} (GeV/#it{c})", "#it{p}_{T,jet}^{det} (GeV/#it{c})", 20., 200., 10., 300.);
    frame->GetXaxis()->SetTitleOffset(1.2);
    double axissize = 0.045;
    frame->GetXaxis()->SetTitleSize(axissize);
    frame->GetYaxis()->SetTitleSize(axissize);
    frame->GetXaxis()->SetLabelSize(axissize);
    frame->GetYaxis()->SetLabelSize(axissize);
    frame->Draw("axis");
    responsematrix->GetZaxis()->SetRangeUser(1e-10, 1);
    responsematrix->SetDirectory(nullptr);
    responsematrix->Draw("colzsame");
    auto label = new ROOT6tools::TNDCLabel(0.15, 0.7, 0.75, 0.94, "pp #sqrt{s} = 13 TeV, PYTHIA8(Monash2013)");
    label->AddText(Form("Anti-#it{k}_{T}, #it{R}=%.1f", double(radius)/10.));
    label->AddText("#it{p}_{T}^{track} > 0.15 GeV/#it{c}, #it{E}^{cluster} > 0.3 GeV");
    label->AddText(Form("|#it{#eta}^{track,cluster}| < 0.7, |#it{#eta}^{jet}| < %.1f", 0.7-double(radius)/10.));
    label->SetTextAlign(12);
    label->SetTextSize(0.045);
    label->Draw();
    auto alilabel = new ROOT6tools::TNDCLabel(0.15, 0.6, 0.5, 0.7, "ALICE Simulation");
    alilabel->SetTextAlign(12);
    alilabel->SetTextSize(0.045);
    alilabel->Draw();

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}