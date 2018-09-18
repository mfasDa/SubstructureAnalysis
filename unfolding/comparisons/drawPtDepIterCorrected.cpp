#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"

struct PtBin {
    double ptmin;
    double ptmax;
    TH1 *spectrum;

    bool operator==(const PtBin &other) const { return TMath::Abs(ptmin - other.ptmin) < DBL_EPSILON && TMath::Abs(ptmax - other.ptmax) < DBL_EPSILON; }
    bool operator<(const PtBin &other) const { return ptmax <= other.ptmin; }
};

std::set<PtBin> readFile(const std::string_view filename, const std::string_view trigger, int iter, double cutptmin = -1., double cutptmax = 1000.) {
    std::set<PtBin> bins;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto keys = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
    reader->cd(Form("iteration%d", iter));
    std::unique_ptr<TH2> h2(static_cast<TH2 *>(gDirectory->Get(Form("zg_unfolded_iter%d", iter))));
    h2->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, h2->GetYaxis()->GetNbins())){
        auto ptmin = h2->GetYaxis()->GetBinLowEdge(b+1), ptmax = h2->GetYaxis()->GetBinUpEdge(b+1);
        if(ptmax < cutptmin || ptmin > cutptmax) continue;
        auto hpro = h2->ProjectionX(Form("correctedzg_%d_%d_%s", static_cast<int>(ptmin), static_cast<int>(ptmax), trigger.data()), b+1, b+1);
        hpro->SetDirectory(nullptr);
        // Correct for efficiency
        auto eff = (*std::find_if(keys.begin(), keys.end(), [ptmin, ptmax](const TKey *k) { return std::string(k->GetName()) == std::string(Form("efficiency_%d_%d", int(ptmin), int(ptmax))); } ))->ReadObject<TH1>();
        std::cout << "Using " << eff->GetName() << " for correction of the kinematic efficiency of " << hpro->GetName() << std::endl;
        hpro->Divide(eff);
        hpro->Scale(1./hpro->Integral());
        normalizeBinWidth(hpro);
        bins.insert({ptmin, ptmax, hpro});
    }
    return bins;
}

void drawPtDepIterCorrected(double r, int iter, bool rangecutTrigger){
    std::map<std::string, std::pair<double, double>> ptranges = {{"INT7", {30., 80.}}, {"EJ2", {80., 120.}}, {"EJ1", {120., 200.}}};
    auto plot = new ROOT6tools::TSavableCanvas(Form("rawZgCorrected_R%02d_iter%d", int(r*10.), iter), Form("Zg unfolded iter%d", iter), 800, 600);
    plot->cd();
    //(new ROOT6tools::TAxisFrame("zgframe", "z_{g}", "1/N_{jet} N_{jet}(z_{g})" , 0., 0.55, 0., 0.6))->Draw("axis");
    (new ROOT6tools::TAxisFrame("zgframe", "z_{g}", "1/N_{jet} dN_{jet}/dz_{g}" , 0., 0.55, 0., 10.))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.85, 0.35, 0.89, Form("jets, anti-kt, R=%.1f", r)))->Draw();
    (new ROOT6tools::TNDCLabel(0.15, 0.79, 0.35, 0.83, Form("%d iterations", iter)))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.45, 0.4, 0.89, 0.89);
    leg->Draw();
    std::map<std::string, Color_t> colors = {{"INT7", kRed}, {"EJ1", kGreen+2}, {"EJ2", kBlue}};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 26, 37, 38, 39, 40};
    std::vector<std::string> triggersequence = {"INT7", "EJ2", "EJ1"};
    for(auto t : triggersequence) {
        auto col = colors.find(t)->second;
        std::stringstream filename;
        filename << "JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_" << t << "_unfolded_zg.root";
        double cutmin = -1., cutmax = 1000.;
        if(rangecutTrigger) {
            auto cuts =  ptranges.find(t)->second;
            cutmin = cuts.first+0.5;
            cutmax = cuts.second-0.5;
        }
        auto hists = readFile(filename.str(), t, iter, cutmin, cutmax);
        int ihist = 0;
        for(auto h : hists) {
            Style{col, markers[ihist++]}.SetStyle<TH1>(*h.spectrum);
            h.spectrum->Draw("epsame");
            leg->AddEntry(h.spectrum, Form("%s, %.1f GeV/c < p_{t} < %.1f GeV/c", t.data(), h.ptmin, h.ptmax));
        }
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}