#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"

struct legentryThresholds {
    TH1         *fHist;
    std::string fTriggerText;
    double      fCentralValue;
    double      fUncertainty;

    void AddToLegend(TLegend *leg) {
        leg->AddEntry(fHist, Form("%s: %.1f #pm %.1f", fTriggerText.data(), fCentralValue, fUncertainty), "lep");
    }
};

void makePlotTriggerPerf(const std::string_view inputfile = "triggerturnon.root"){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));

    std::map<int, Style> stylesHigh = {{2, {kRed, 24}}, {3, {kGreen+2, 25}}, {4, {kBlue, 26}}, {5, {kViolet, 27}}};
    std::map<int, Style> stylesLow = {{2, {kRed, 20}}, {3, {kGreen+2, 21}}, {4, {kBlue, 22}}, {5, {kViolet, 23}}};

    auto plot = new ROOT6tools::TSavableCanvas("PerfEmcalTrigger", "Performance of the EMCAL jet trigger", 800, 600);
    plot->cd();
    gPad->SetLogy();

    (new ROOT6tools::TAxisFrame("EJ1frame", "p_{t,jet} (GeV/c)", "Triggered / Min. bias", 0., 200., 0.8, 100000))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.79, 0.87, 0.87, "ALICE preliminary, pp #sqrt{s} = 13 TeV, #intLdt = 4 pb^{-1}, jets, anti-k_{t}"))->Draw();
    auto legR = new ROOT6tools::TDefaultLegend(0.7, 0.15, 0.89, 0.4);
    auto legT = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.6, 0.23);
    legR->Draw();
    legT->Draw();
    legentryThresholds ej1entry, ej2entry; 
    reader->cd("ej1turnon");
    auto histsj1 = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
    for(double r = 0.2; r < 0.6; r+=0.1){
        auto histkey = *std::find_if(histsj1.begin(), histsj1.end(), [r](const TKey *k) { return contains(k->GetName(), Form("R%02d", int(r*10.))); });
        auto hist = histkey->ReadObject<TH1>();
        hist->SetDirectory(nullptr);
        stylesHigh.find(int(r*10))->second.SetStyle<TH1>(*hist);
        hist->Draw("epsame");
        legR->AddEntry(hist, Form("R=%.1f", r), "lep");
        if(TMath::Abs(r-0.2) < DBL_EPSILON) {
          ej1entry.fHist = hist;
          ej1entry.fTriggerText = "high threshold";
        }
        if(TMath::Abs(r-0.3) < DBL_EPSILON) {
            auto model = new TF1("modelEJ1", "pol0", 0., 200.);
            model->SetLineColor(kRed);
            model->SetLineStyle(2);
            hist->Fit(model, "N", "", 80., 200.);
            model->Draw("lsame");
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
          ej2entry.fHist = hist;
          ej2entry.fTriggerText = "low threshold";
        }
        if(TMath::Abs(r-0.3) < DBL_EPSILON) {
            auto model = new TF1("modelEJ2", "pol0", 0., 200.);
            model->SetLineColor(kRed);
            model->SetLineStyle(3);
            hist->Fit(model, "N", "", 60., 200.);
            model->Draw("lsame");
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