#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/substructuretree.C"

void plotStatsFromUnfoldingOutput(const std::string_view inputfile){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    auto hraw = static_cast<TH2 *>(reader->Get("hraw"));
    hraw->SetDirectory(nullptr);

    auto tag = getFileTag(inputfile);
    auto jd = getJetType(tag);

    auto plot = new ROOT6tools::TSavableCanvas(Form("stat_%s", tag.data()), Form("statistics %s", tag.data()), 800, 600);
    plot->cd();
    hraw->SetXTitle("z_{g}");
    hraw->SetYTitle("p_{t} (GeV/c");
    hraw->SetTitle(Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data()));
    hraw->SetStats(false);
    hraw->Draw("text");
    plot->SaveCanvas(plot->GetName());
}