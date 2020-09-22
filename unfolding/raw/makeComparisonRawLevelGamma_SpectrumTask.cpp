#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"

std::vector<double> getJetPtBinningNonLinSmearLarge(){
  std::vector<double> result;
  result.emplace_back(20.);
  double current = 20.;
  while(current < 40.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 60){
    current += 5.;
    result.push_back(current);
  }
  while(current < 120){
    current += 10.;
    result.push_back(current);
  }
  while(current < 200){
    current += 20.;
    result.push_back(current);
  }
  return result;
}

void DrawSpectra(const std::map<std::string, TH1 *> &spectra, Double_t r, Bool_t doleg, Bool_t rebinned) {
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame(Form("axispec_R%02d_%s", int(r*10), (rebinned ? "rebinned" : "fine")), "p_{t} (GeV/c)", "dN/dp_{t} ((GeV/c)^{-1})", 0., 200, 1, 1e9))->Draw("axis");
    TLegend *leg(nullptr);
    if(doleg) {
        leg = new ROOT6tools::TDefaultLegend(0.7, 0.65, 0.89, 0.89);
        leg->Draw();
    }
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("R=%.2f", r)))->Draw();
    for(auto [t, s] : spectra) {
        s->Draw("epsame");
        if(leg) leg->AddEntry(s, t.data(), "lep");
    }
    gPad->Update();
}

void DrawRatios(const std::map<std::string, TH1 *> &spectra, Double_t r, Bool_t rebinned) {
    (new ROOT6tools::TAxisFrame(Form("ratioTriggerMinBias_R%02d%s", int(r*10), (rebinned ? "rebinned" : "fine")), "p_{t} (GeV/c)", "Trigger / min. bias", 0., 200., 0., 2.))->Draw("axis");
    TLegend *leg(nullptr);
    if(rebinned) {
        leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.89, 0.3);
        leg->Draw();
    }
    std::array<std::string, 2> emcaltriggers = {{"EG1", "EG2"}};
    auto ref = spectra.find("INT7")->second;
    for(const auto &et : emcaltriggers){
        auto ratio = histcopy(spectra.find(et)->second);
        ratio->SetDirectory(nullptr);
        ratio->Divide(ref);
        ratio->Draw("epsame");
        if(leg) {
            auto model = new TF1(Form("modelR%02d_%s", int(r*10.), et.data()), "pol0", 80., 200);
            ratio->Fit(model, "N", "", 80., 200.);
            model->SetLineColor(ratio->GetLineColor());
            model->SetLineStyle(2);
            model->Draw("lsame");
            leg->AddEntry(model, Form("%s: %.2f #pm %.2f", et.data(), model->GetParameter(0), model->GetParError(0)), "l");
        }
    }
    gPad->Update();
}

void makeComparisonRawLevelGamma_SpectrumTask(const std::string_view corrfile, const std::string sysvar) {
    auto plotfine = new ROOT6tools::TSavableCanvas("comparisonTriggersRaw_finebinning", "Comparison triggers (fine binning)", 1500, 700),
         plotrebinned = new ROOT6tools::TSavableCanvas("comparisonTriggersRaw_rebinned", "Comparison triggers (rebinned)", 1500, 700);
    plotfine->Divide(5,2);
    plotrebinned->Divide(5,2);

    std::unique_ptr<TFile> reader(TFile::Open(corrfile.data(), "READ"));
    std::array<std::string, 3> triggers = {{"INT7", "EG1", "EG2"}};
    std::map<std::string, std::string> clusters = {{"INT7", "ANY"}, {"EG1", "CENTNOTRD"}, {"EG2", "CENT"}};
    std::map<std::string, Style> styles = {{"INT7", {kBlack, 20}},{"EG1", {kRed, 24}}, {"EG2", {kBlue, 25}}};
    auto binningCoarse = getJetPtBinningNonLinSmearLarge();
    int icol = 1;
    std::vector<int> jetr = {0, 1, 2, 3, 4};
    for(auto r : jetr){
        reader->cd(Form("R%02d", r));
        gDirectory->cd("rawlevel");
        std::map<std::string, TH1 *> triggersFine, triggersRebinned;
        for(const auto t : triggers) {
            auto specfine = static_cast<TH1 *>(gDirectory->Get(Form("RawJetSpectrum_FullJets_R%02d_%s_%s_%s", r, t.data(), clusters.find(t)->second.data(), sysvar.data()))),
                 specrebinned = specfine->Rebin(binningCoarse.size()-1, Form("%srebinned_R%02d", t.data(), r), binningCoarse.data());
            normalizeBinWidth(specrebinned);
            specfine->SetName(Form("%sfine_R%02d", t.data(), r));
            auto trgstyle = styles[t];
            trgstyle.SetStyle<TH1>(*specfine);
            trgstyle.SetStyle<TH1>(*specrebinned);
            specfine->SetDirectory(nullptr);
            specrebinned->SetDirectory(nullptr);
            triggersFine[t] = specfine;
            triggersRebinned[t] = specrebinned;
        }

        plotfine->cd(icol);
        double rval = (r == 0) ? 0.05 : double(r)/10.;
        DrawSpectra(triggersFine, rval, icol == 1, false);
        plotrebinned->cd(icol);
        DrawSpectra(triggersRebinned, rval, icol == 1, true);

        plotfine->cd(icol+5);
        DrawRatios(triggersFine, rval, false);
        plotrebinned->cd(icol+5);
        DrawRatios(triggersRebinned, rval, true);
        icol++;
    }
    plotfine->cd();
    plotfine->Update();
    plotfine->SaveCanvas(plotfine->GetName());
    plotrebinned->cd();
    plotrebinned->Update();
    plotrebinned->SaveCanvas(plotrebinned->GetName());
}