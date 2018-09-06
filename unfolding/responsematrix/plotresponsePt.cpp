#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/string.C"

int extractR(const std::string_view filename){
    auto tokens = tokenize(std::string(filename), '_');
    return std::stoi(tokens[2].substr(1));
}

void plotresponsePt(const std::string_view filename){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto responsematrix = static_cast<TH2 *>(reader->Get("matrixfine_allptsim"));

    auto radius = extractR(filename);
    auto plot = new ROOT6tools::TSavableCanvas(Form("responsematrixpt_R%02d",radius), Form("Response matrix, jet radius R=%.1f", double(radius)/10.), 800, 600);
    plot->SetLogz();
    plot->SetRightMargin(0.14);
    responsematrix->GetZaxis()->SetRangeUser(1e-10, 1);
    responsematrix->SetDirectory(nullptr);
    responsematrix->SetStats(false);
    responsematrix->SetTitle("");
    responsematrix->Draw("colz");
    (new ROOT6tools::TNDCLabel(0.15, 0.9, 0.45, 0.97, Form("Full jets, R=%.1f", double(radius)/10.)))->Draw();

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}