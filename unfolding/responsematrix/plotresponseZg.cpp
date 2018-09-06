#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/string.C"

int extractR(const std::string_view filename){
    auto tokens = tokenize(std::string(filename), '_');
    return std::stoi(tokens[2].substr(1));
}

void plotresponseZg(const std::string_view filename, double ptmin, double ptmax){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto responsematrix = static_cast<TH2 *>(reader->Get(Form("matrixfine_%d_%d", int(ptmin), int(ptmax))));

    auto radius = extractR(filename);
    auto plot = new ROOT6tools::TSavableCanvas(Form("responsematrixzg_R%02d_%d_%d",radius, int(ptmin), int(ptmax)), Form("Response matrix, jet radius R=%.1f", double(radius)/10.), 800, 600);
    plot->SetLogz();
    plot->SetRightMargin(0.14);
    responsematrix->GetZaxis()->SetRangeUser(1e-10, 1e-4);
    responsematrix->SetDirectory(nullptr);
    responsematrix->SetStats(false);
    responsematrix->SetTitle("");
    responsematrix->Draw("colz");
    (new ROOT6tools::TNDCLabel(0.15, 0.9, 0.7, 0.97, Form("Full jets, R=%.1f, %.1f GeV/c < p_{t} < %.1f GeV/c", double(radius)/10., ptmin, ptmax)))->Draw();

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}