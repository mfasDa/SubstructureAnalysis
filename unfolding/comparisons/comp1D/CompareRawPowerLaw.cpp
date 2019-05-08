#include "../../../struct/JetSpectrumReader.cxx"
#include "../../../struct/GraphicsPad.cxx"

Double_t
LevyTsallis_Func(const Double_t *x, const Double_t *p)
{
  /* dN/dpt */

  Double_t pt = x[0];
  Double_t mass = p[0];
  Double_t mt = TMath::Sqrt(pt * pt + mass * mass);
  Double_t n = p[1];
  Double_t C = p[2];
  Double_t norm = p[3];

  Double_t part1 = (n - 1.) * (n - 2.);
  Double_t part2 = n * C * (n * C + mass * (n - 2.));
  Double_t part3 = part1 / part2;
  Double_t part4 = 1. + (mt - mass) / n / C;
  Double_t part5 = TMath::Power(part4, -n);
  return pt * norm * part3 * part5;
}

Double_t
TrueTsallis_Func(const Double_t *x, Double_t *p)
{
    /* dN/dpt */
    Double_t pt = x[0];
    Double_t mass = p[0];
    Double_t mt = TMath::Sqrt(pt * pt + mass * mass);
    Double_t q = p[1];
    Double_t T = p[2];
    Double_t dNdy = p[3];

    Double_t part1 = pt*mt;
    Double_t part2 = part1/T;
    Double_t part3 = (2.-q)*(3.-2.*q);
    Double_t part4 = (2.-q)*mass*mass + 2.*mass*T +2.*T*T;
    Double_t part5 = part3/part4;

    Double_t part6 = 1. + (q-1.)*mass/T;
    Double_t part7 = TMath::Power(part6, 1./(q-1.));

    Double_t part8 = 1. + (q-1.)*mt/T;
    Double_t part9 = TMath::Power(part8, -q/(q-1.));
    return part2 * dNdy * part5 * part7 * part9;
}

void CompareRawPowerLaw(const std::string_view filename){
    std::vector<std::string> spectra = {"hraw"};
    auto data = JetSpectrumReader(filename, spectra);
    auto jetradii = data.GetJetSpectra().GetJetRadii();
    
    Style specstyle{kBlack, 20};
    auto plot = new ROOT6tools::TSavableCanvas("fitRawPowerlaw", "Fit of the raw spectrum to power law", 300 * jetradii.size(), 700);
    plot->Divide(jetradii.size(), 2);
    int icol(0);
    for(auto r : jetradii){
        plot->cd(icol+1);
        GraphicsPad specpad(gPad);
        specpad.Logy();
        specpad.Frame(Form("specpad_R%02d",int(r*10.)), "p_{t} (GeV/c)", "1/N_{ev} dN/dp_{t} ((GeV/c)^{-1})", 0., 250, 1e-11, 1e-2);
        specpad.Label(0.15, 0.15, 0.4, 0.22, Form("R=%.1f", r));
        auto spec = data.GetJetSpectrum(r, spectra[0]);
        specpad.Draw(spec, specstyle);

        //auto model = new TF1(Form("powerlawR%02d", int(r*10.)), "[0] * TMath::Power(2*x/[2],[1])", 0., 250.);
        auto model = new TF1(Form("powerlawR%02d", int(r*10.)), "[0] * TMath::Power(x,[1])", 0., 250.);
        //auto model = new TF1(Form("modifiedHagedornR%02d", int(r*10.)), "[0]/TMath::Power(TMath::Exp(-[1]*x - [2]*x*x) + x/[3], [4])", 0., 250.);
        //auto model = new TF1(Form("tsallis%02d", int(r*10.)),TrueTsallis_Func, 0., 250., 4);

        model->SetParLimits(0, 5e3, 2e4);
        model->SetParameter(0, 1e4);
        model->SetParLimits(1, -5.5, -5.0);
        model->SetParameter(1, -5.2);
        
        //spec->Fit(model, "N", "", 10., 240.);
        spec->Fit(model, "NL", "", 100., 240.);
        model->SetLineColor(kRed);
        model->Draw("lsame");

        plot->cd(icol+jetradii.size()+1);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("ratiopad_R%02d",int(r*10.)), "p_{t} (GeV/c)", "data/fit", 0., 250, 0.5, 1.5);
        auto radafi = (TH1 *)spec->Clone(Form("radafi_R%02d", int(r*10.)));
        for(auto b : ROOT::TSeqI(0, radafi->GetXaxis()->GetNbins())){
            double funval = model->Eval(radafi->GetXaxis()->GetBinCenter(b+1));
            radafi->SetBinContent(b+1, spec->GetBinContent(b+1)/funval);
            radafi->SetBinError(b+1, spec->GetBinError(b+1)/funval);
        }
        ratiopad.Draw<TH1>(radafi, specstyle);
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}