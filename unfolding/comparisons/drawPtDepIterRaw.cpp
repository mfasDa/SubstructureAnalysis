#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

struct PtBin {
    double ptmin;
    double ptmax;
    TH1 *spectrum;

    bool operator==(const PtBin &other) const { return TMath::Abs(ptmin - other.ptmin) < DBL_EPSILON && TMath::Abs(ptmax - other.ptmax) < DBL_EPSILON; }
    bool operator<(const PtBin &other) const { return ptmax <= other.ptmin; }
};

std::set<PtBin> readFile(const std::string_view filename, const std::string_view trigger, int iter) {
    std::set<PtBin> bins;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd(Form("iteration%d", iter));
    std::unique_ptr<TH2> h2(static_cast<TH2 *>(gDirectory->Get(Form("zg_unfolded_iter%d", iter))));
    h2->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, h2->GetYaxis()->GetNbins())){
        auto hpro = h2->ProjectionX(Form("unfzg_%d_%d_%s", static_cast<int>(h2->GetYaxis()->GetBinLowEdge(b+1)), static_cast<int>(h2->GetYaxis()->GetBinUpEdge(b+1)), trigger.data()), b+1, b+1);
        hpro->SetDirectory(nullptr);
        hpro->Scale(1./hpro->Integral());
        bins.insert({h2->GetYaxis()->GetBinLowEdge(b+1), h2->GetYaxis()->GetBinUpEdge(b+1), hpro});
    }
    return bins;
}

void drawPtDepIterRaw(double r, int iter){
    auto plot = new ROOT6tools::TSavableCanvas(Form("rawZgUnfolded_R%02d_iter%d", int(r*10.), iter), Form("Zg unfolded iter%d", iter), 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("zgframe", "z_{g}", "1/N_{jet} N_{jet}(z_{g})" , 0., 0.55, 0., 0.6))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.85, 0.35, 0.89, Form("jets, anti-kt, R=%.1f", r)))->Draw();
    (new ROOT6tools::TNDCLabel(0.15, 0.79, 0.35, 0.83, Form("%d iterations", iter)))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.45, 0.3, 0.89, 0.89);
    leg->Draw();
    std::map<std::string, Color_t> colors = {{"INT7", kRed}, {"EJ1", kGreen}, {"EJ2", kBlue}};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 26, 37, 38, 39, 40};
    for(auto t : colors) {
        std::stringstream filename;
        filename << "JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_" << t.first << "_unfolded_zg.root";
        auto hists = readFile(filename.str(), t.first, iter);
        int ihist = 0;
        for(auto h : hists) {
            Style{t.second, markers[ihist++]}.SetStyle<TH1>(*h.spectrum);
            h.spectrum->Draw("epsame");
            leg->AddEntry(h.spectrum, Form("%s, %.1f GeV/c < p_{t} < %.1f GeV/c", t.first.data(), h.ptmin, h.ptmax));
        }
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}