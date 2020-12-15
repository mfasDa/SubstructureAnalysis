#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/Restrictor.cxx"

void ZeroStat(TH1 *hist) {
    for(auto b : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())) {
        hist->SetBinError(b+1, 0);
    }
}

void makePlotCorrelatedUncertaintiesRatiosPt1D(const std::string_view ufile){
    Restrictor reported(15, 320);
    auto plot = new ROOT6tools::TSavableCanvas("correlatedUncertaintiesRatios", "Correlated Uncertainties", 1200, 800);
    plot->Divide(2,2);

    std::map<std::string, Color_t> styles = {{"clusterizer", kRed}, {"hadcorr", kViolet}, {"nonlin", kTeal}, {"thresholds", kMagenta}, {"time",kBlue}, {"trackingeff", kGreen}, {"trkmatch", kYellow}, {"trigger", kGray}};
    std::map<std::string, std::string> titles {{"clusterizer", "Clusterizer"}, {"hadcorr", "Had. Corr."}, {"nonlin", "Non. lin. corr"}, {"thresholds", "En. thresholds"}, {"time", "Lead. cluster time"}, {"trackingeff", "Tracking eff."}, {"trkmatch", "Clust-track matcher"}, {"trigger", "Trigger efficiency"}};

    std::unique_ptr<TFile> reader(TFile::Open(ufile.data(), "READ"));
    int ipad = 0;
    for(double r = 0.3; r <= 0.6; r += 0.1) {
        plot->cd(ipad+1);
        GraphicsPad syspad(gPad);
        syspad.Margins(0.13, 0.05, -1., 0.05);
        syspad.Frame(Form("sysR%02d", int(r*10.)), "p_{t} (GeV/c)", "sys. uncertainty", 0., 350., 0., 0.3);
        syspad.Label(0.7, 0.8, 0.89, 0.89, Form("R=0.2/R=%.1f", r));
        TLegend *leg(nullptr);
        if(ipad == 0) {
            leg = new ROOT6tools::TDefaultLegend(0.15, 0.5, 0.7, 0.89);
            leg->Draw();   
        }
        reader->cd(Form("R02R%02d", int(r*10)));
        auto combined = reported(static_cast<TH1 *>(gDirectory->Get("combinedUncertainty")));
        combined->SetDirectory(nullptr);
        ZeroStat(combined);
        combined->SetLineColor(kBlack);
        combined->Draw("same");
        if(leg) leg->AddEntry(combined, "sum", "l");
        for(auto [source, style] : styles) {
            auto uncertainty = reported(static_cast<TH1 *>(gDirectory->Get(Form("uncertainty%s", source.data()))));
            uncertainty->SetDirectory(nullptr);
            ZeroStat(uncertainty);
            uncertainty->SetLineColor(style);
            uncertainty->Draw("same");
            if(leg) leg->AddEntry(uncertainty, titles.find(source)->second.data(), "lep");
        }
        ipad++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}