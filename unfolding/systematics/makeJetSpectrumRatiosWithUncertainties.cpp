#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/Restrictor.cxx"

TGraphErrors *makeErrors(TH1 *stat, TH1 *sys){
    auto graph = new TGraphErrors;
    int np(0);
    for(auto b : ROOT::TSeqI(0, sys->GetXaxis()->GetNbins())){
        auto val = stat->GetBinContent(b+1);
        graph->SetPoint(np, sys->GetXaxis()->GetBinCenter(b+1), val);
        graph->SetPointError(np, sys->GetXaxis()->GetBinWidth(b+1)/2., val * sys->GetBinContent(b+1));
        np++;
    }
    return graph;
}

void makeJetSpectrumRatiosWithUncertainties(const std::string_view corr, const std::string_view shape){
    std::unique_ptr<TFile> correader(TFile::Open(corr.data(), "READ")), shapereader(TFile::Open(shape.data(), "READ"));
    Restrictor reported(10, 320);
    
    auto plot = new ROOT6tools::TSavableCanvas("jetSpectrumWithErrors", "Jet spectrum", 1200, 800);
    plot->Divide(2,2);

    std::vector<Color_t> colors = {kRed, kBlue, kGreen, kOrange, kViolet};
    
    int ipad = 0;
    for(double r = 0.3; r <= 0.6; r+=0.1) {
        std::string rstring(Form("R%02d", int(r*10)));
        std::string dirstring(Form("R02R%02d", int(r*10)));
        plot->cd(ipad+1);
        gPad->SetLeftMargin(0.15);
        auto frame = new ROOT6tools::TAxisFrame(Form("frameR%02d", int(r*10.)), "p_{t} (GeV/c)", "d#sigma(R=0.2)/(dp_{t}dy) / d#sigma(R=X)/(dp_{t}dy) ", 0, 350, 1e-9, 1);
        frame->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.4, 0.89, Form("R=0.2/R=%.1f", r)))->Draw();
        TLegend *leg(nullptr);
        if(!ipad) {
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.5);
            leg->Draw();
        }
        correader->cd(dirstring.data());
        auto spec = reported(static_cast<TH1*>(gDirectory->Get("DefaultRatio"))),
              ucorr = reported(static_cast<TH1 *>(gDirectory->Get("combinedUncertainty")));
        shapereader->cd(dirstring.data());
        auto ushape = reported(static_cast<TH1 *>(gDirectory->Get("combinedUncertainty")));
        auto plotCorr = makeErrors(spec, ucorr), plotShape = makeErrors(spec, ushape);
        spec->SetMarkerStyle(24);
        spec->SetMarkerColor(colors[ipad]);
        spec->SetLineColor(colors[ipad]);
        spec->Draw("epsame");
        plotCorr->SetLineColor(colors[ipad]);
        plotCorr->SetFillStyle(0);
        plotShape->SetFillColor(colors[ipad]);
        plotShape->SetFillStyle(3001);
        plotCorr->Draw("2same");
        plotShape->Draw("2same");
        if(leg) {
            leg->AddEntry(spec, "Stat", "lep");
            leg->AddEntry(plotCorr, "Corr. Uncertainty", "F");
            leg->AddEntry(plotShape, "Shape. Uncertainty", "F");
        }
        ipad++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}