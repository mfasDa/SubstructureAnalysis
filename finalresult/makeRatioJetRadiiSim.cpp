#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

TGraphErrors *readTheorySpectrum(const std::string_view filename, double r){
    TGraphErrors *spectrum = new TGraphErrors;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto hist = static_cast<TH1 *>(reader->Get("jetptspectrum"));
    int np(0);
    for(auto b : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())) {
        if(hist->GetXaxis()->GetBinUpEdge(b+1) > 250.) break;
        if(hist->GetXaxis()->GetBinLowEdge(b+1) <30.) continue;
        spectrum->SetPoint(np, hist->GetXaxis()->GetBinCenter(b+1), hist->GetBinContent(b+1));
        spectrum->SetPointError(np, hist->GetXaxis()->GetBinWidth(b+1)/2., hist->GetBinError(b+1));
        np++;
    }
    return spectrum;
}

TGraphErrors *ratiocentral(TGraphErrors *num, TGraphErrors *den) {
    TGraphErrors *result = new TGraphErrors;
    for(auto p : ROOT::TSeqI(0, num->GetN())){
        result->SetPoint(p, num->GetX()[p], num->GetY()[p]/den->GetY()[p]);
        result->SetPointError(p, num->GetEX()[p], num->GetY()[p]/den->GetY()[p]*TMath::Sqrt(TMath::Power(num->GetEY()[p]/num->GetY()[p],2) + TMath::Power(den->GetEY()[p]/den->GetY()[p], 2)));
    }    
    return result;
}

void makeRatioJetRadiiSim(){
    /*
    auto specr2 = readTheorySpectrum("spectrumPerugia11_R02.root", 0.2), 
         specr4 = readTheorySpectrum("spectrumPerugia11_R04.root", 0.4),
         specr5 = readTheorySpectrum("spectrumPerugia11_R05.root", 0.5);
         */
    auto specr2 = readTheorySpectrum("spectrumFine_Perugia11_R02.root", 0.2), 
         specr4 = readTheorySpectrum("spectrumFine_Perugia11_R04.root", 0.4),
         specr5 = readTheorySpectrum("spectrumFine_Perugia11_R05.root", 0.5);

    auto R0204 = ratiocentral(specr2, specr4), R0205 = ratiocentral(specr2, specr5);
    auto plot = new ROOT6tools::TSavableCanvas("ratiojetradiiSim", "Ratio jet radii", 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("ratioframe", "p_{t} (GeV/c)", "Ratio jet radii", 0., 250., 0., 1.2))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.3);
    leg->Draw();
    Style{kRed, 24}.SetStyle<TGraphErrors>(*R0204);
    Style{kBlue, 25}.SetStyle<TGraphErrors>(*R0205);
    R0204->Draw("epsame");
    R0205->Draw("epsame");
    leg->AddEntry(R0204, "R=0.2/R=0.4", "lep");
    leg->AddEntry(R0205, "R=0.2/R=0.5", "lep");
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}