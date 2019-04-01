#include "../../binnings/binningPt1D.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"

std::map<double, TH1 *> readRawSpectra(const std::string_view filename){
    auto binning = getJetPtBinningNonLinSmearLargeFineLow();
    TH1D histtemplate("histtemplate", "histtemplate", binning.size()-1, binning.data());
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(double r = 0.2; r < 0.7; r+= 0.1) {
        std::string rstring = Form("R%02d", int(r*10.));
        reader->cd(rstring.data());
        gDirectory->cd("rawlevel");
        auto spectrumtmp = static_cast<TH1 *>(gDirectory->Get(Form("hraw_%s", rstring.data())));
        auto spectrum = new TH1D(histtemplate);
        spectrum->SetDirectory(nullptr);
        spectrum->SetNameTitle(spectrumtmp->GetName(), spectrumtmp->GetTitle());
        for(auto b : ROOT::TSeqI(0, spectrum->GetXaxis()->GetNbins())) {
            auto tmpbin = spectrumtmp->GetXaxis()->FindBin(spectrum->GetXaxis()->GetBinCenter(b+1));
            spectrum->SetBinContent(b+1, spectrumtmp->GetBinContent(tmpbin));
            spectrum->SetBinError(b+1, spectrumtmp->GetBinError(tmpbin));
        }
        spectrum->Scale(1., "width");
        result[r] = spectrum;
    }
    return result;
}

void makeComparisonRawRestricted(const std::string_view fileultra, const std::string_view filereg){
    auto dataUltra = readRawSpectra(fileultra), datareg = readRawSpectra(filereg);

    auto plot = new ROOT6tools::TSavableCanvas("comparisonRawRanges", "Comparison raw spectra in identical range", 300 * dataUltra.size(), 700);
    plot->Divide(dataUltra.size(),2);

    Style ultrastyle{kRed, 24}, regstyle{kBlue, 25}, ratiostyle{kBlack, 20};
    int icol = 0;
    for(auto ultrait = dataUltra.begin(), regit = datareg.begin(); ultrait != dataUltra.end(); ++ultrait, ++regit){
        plot->cd(icol+1);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(ultrait->first * 10.)), "p_{t} (GeV/c)", "dN/dp_{t}", 0., 250, 1e-9, 10))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.3, 0.22, Form("R=%.1f", ultrait->first)))->Draw();
        TLegend *leg(nullptr);
        if(!icol){
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
            leg->Draw();
        }
        ultrastyle.SetStyle<TH1>(*ultrait->second);
        regstyle.SetStyle<TH1>(*regit->second);
        ultrait->second->Draw("epsame");
        regit->second->Draw("epsame");
        if(leg) {
            leg->AddEntry(regit->second, "Normal range", "lep" );
            leg->AddEntry(ultrait->second, "Wide range", "lep" );
        }
        plot->cd(icol+dataUltra.size()+1);
        (new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(ultrait->first * 10.)), "p_{t} (GeV/c)", "wide/normal", 0., 250, 0.5, 1.5))->Draw("axis");
        auto ratio = static_cast<TH1 *>(ultrait->second->Clone(Form("ratioR%02d", int(ultrait->first * 10.))));
        ratio->SetDirectory(nullptr);
        ratio->Divide(regit->second);
        ratiostyle.SetStyle<TH1>(*ratio);
        ratio->Draw("epsame");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}