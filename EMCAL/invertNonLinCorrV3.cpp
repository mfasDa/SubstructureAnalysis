#include "../meta/root.C"
#include "../meta/root6tools.C"

void invertNonLinCorrV3(){
    std::array<double, 7> fNonLinearityParams = {{0.976941, 0.162310, 1.08689, 0.0819592, 152.338, 30.9594, 0.9615}};
    auto model = [fNonLinearityParams](double *x, double *p) { return fNonLinearityParams[6]/(fNonLinearityParams[0]*(1./(1.+fNonLinearityParams[1]*TMath::Exp(-x[0]/fNonLinearityParams[2]))*1./(1.+fNonLinearityParams[3]*TMath::Exp((x[0]-fNonLinearityParams[4])/fNonLinearityParams[5])))); };
    auto nonlincorr = new TF1("nonlinearity", model, 0., 200., 1);

    TGraph *inverted = new TGraph, 
           *correlation = new TGraph;
    int np(0);
    for(double raw = 0.3; raw <= 200.; raw += 0.1) {
        auto evaluated = raw * nonlincorr->Eval(raw);
        inverted->SetPoint(np, evaluated, raw/evaluated);
        correlation->SetPoint(np, raw, evaluated);
        np++;
    }
    std::unique_ptr<TFile> writer(TFile::Open("nonLinCorrInverted.root", "RECREATE"));
    writer->cd();
    inverted->Write("invertedkTestbeamv3");
    correlation->Write("correlation");
}