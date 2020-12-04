#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"

struct legentryThresholds {
    TF1         *fHist;
    std::string fTriggerText;
    double      fCentralValue;
    double      fUncertainty;

    void AddToLegend(TLegend *leg) {
        leg->AddEntry(fHist, Form("%s: %d", fTriggerText.data(), int(fCentralValue)), "l");
    }
};

void makePlotTriggerPerfLin(const std::string_view inputfile = "triggerturnon.root"){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));

    std::map<int, Style> stylesHigh = {{2, {kRed, 24}}, {3, {kGreen+2, 25}}, {4, {kBlue, 26}}, {5, {kViolet, 27}}};
    std::map<int, Style> stylesLow = {{2, {kRed, 20}}, {3, {kGreen+2, 21}}, {4, {kBlue, 22}}, {5, {kViolet, 23}}};

    auto plot = new ROOT6tools::TSavableCanvas("PerfEmcalTriggerLin", "Performance of the EMCAL jet trigger", 800, 600);
    plot->cd();
    gPad->SetTicks();
    gPad->SetRightMargin(0.04);
    gPad->SetTopMargin(0.04);
    gPad->SetLeftMargin(0.13);
    gPad->SetBottomMargin(0.13);

    auto frame = new ROOT6tools::TAxisFrame("EJ1frame", "#it{p}_{T,jet} (GeV/#it{c})", "Yield triggered / Min. bias", 0., 200., 0.0, 15000);
    auto axissize = 0.045;
    frame->GetXaxis()->SetTitleSize(axissize);
    frame->GetYaxis()->SetTitleSize(axissize);
    frame->GetXaxis()->SetLabelSize(axissize);
    frame->GetYaxis()->SetLabelSize(axissize);
    frame->GetXaxis()->SetTitleOffset(1.3);
    frame->Draw("axis");
    auto label  = new ROOT6tools::TNDCLabel(0.15, 0.69, 0.93, 0.90, "ALICE Performance, pp #sqrt{s} = 13 TeV, #intLdt = 4 pb^{-1}");
    label->AddText("Full jets, Anti-#it{k}_{T}, #it{p}_{T}^{track} > 0.15 GeV/#it{c}, #it{E}^{cluster} > 0.3 GeV")->Draw();
    label->AddText("|#it{#eta}^{track,cluster}| < 0.7, |#it{#eta}^{jet}| < 0.7 - #it{R}");
    label->SetTextAlign(12);
    label->Draw();

    auto legR = new ROOT6tools::TDefaultLegend(0.8, 0.15, 0.94, 0.5);
    auto legT = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.6, 0.27);
    legR->Draw();
    legT->Draw();
    legT->SetTextSize(0.045);
    legR->SetTextSize(0.045);
    legentryThresholds ej1entry, ej2entry; 
    reader->cd("ej1turnon");
    auto histsj1 = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
    for(double r = 0.2; r < 0.6; r+=0.1){
        auto histkey = *std::find_if(histsj1.begin(), histsj1.end(), [r](const TKey *k) { return contains(k->GetName(), Form("R%02d", int(r*10.))); });
        auto hist = histkey->ReadObject<TH1>();
        hist->SetDirectory(nullptr);
        stylesHigh.find(int(r*10))->second.SetStyle<TH1>(*hist);
        hist->Draw("epsame");
        legR->AddEntry(hist, Form("#it{R} = %.1f", r), "lep");
        if(TMath::Abs(r-0.2) < DBL_EPSILON) {
          ej1entry.fTriggerText = "high threshold (#it{E} = 20 GeV)";
        }
        if(TMath::Abs(r-0.3) < DBL_EPSILON) {
            auto model = new TF1("modelEJ1", "pol0", 0., 200.);
            model->SetLineColor(kGreen+2);
            model->SetLineStyle(2);
            hist->Fit(model, "N", "", 80., 200.);
            model->Draw("lsame");
            ej1entry.fHist = model;
            ej1entry.fCentralValue = model->GetParameter(0);
            ej1entry.fUncertainty  = model->GetParError(0);
        }
    }

    reader->cd("ej2turnon");
    auto histsj2 = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
    for(double r = 0.2; r < 0.6; r+=0.1){
        auto histkey = *std::find_if(histsj2.begin(), histsj2.end(), [r](const TKey *k) { return contains(k->GetName(), Form("R%02d", int(r*10))); });
        auto hist = histkey->ReadObject<TH1>();
        hist->SetDirectory(nullptr);
        stylesLow.find(int(r*10.))->second.SetStyle<TH1>(*hist);
        hist->Draw("epsame");
        if(TMath::Abs(r-0.2) < DBL_EPSILON)  {
          ej2entry.fTriggerText = "low threshold (#it{E} = 16 GeV)";
        }
        if(TMath::Abs(r-0.3) < DBL_EPSILON) {
            auto model = new TF1("modelEJ2", "pol0", 0., 200.);
            model->SetLineColor(kGreen+2);
            model->SetLineStyle(3);
            hist->Fit(model, "N", "", 60., 200.);
            model->Draw("lsame");
            ej2entry.fHist = model;
            ej2entry.fCentralValue = model->GetParameter(0);
            ej2entry.fUncertainty  = model->GetParError(0);
        }
    }
    ej1entry.AddToLegend(legT);
    ej2entry.AddToLegend(legT);
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}