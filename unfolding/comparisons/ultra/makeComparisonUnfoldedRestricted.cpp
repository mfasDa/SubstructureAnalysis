#include "../../binnings/binningPt1D.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"

std::map<double, TH1 *> readRawSpectra(const std::string_view filename){
    auto binningtmp = getJetPtBinningNonLinTrueLargeFineLow();
    std::vector<double> binning;
    // restrict to reported range of the measurement with the regular binning
    for(auto b : binningtmp) {
        if(b < 20.) continue;
        if(b > 240.) break;
        binning.push_back(b);
    }
    TH1D histtemplate("histtemplate", "histtemplate", binning.size()-1, binning.data());
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(double r = 0.2; r < 0.7; r+= 0.1) {
        std::string rstring = Form("R%02d", int(r*10.));
        reader->cd(rstring.data());
        gDirectory->cd("reg4");
        auto spectrumtmp = static_cast<TH1 *>(gDirectory->Get("normalized_reg4"));
        auto spectrum = new TH1D(histtemplate);
        spectrum->SetDirectory(nullptr);
        spectrum->SetNameTitle(spectrumtmp->GetName(), spectrumtmp->GetTitle());
        for(auto b : ROOT::TSeqI(0, spectrum->GetXaxis()->GetNbins())) {
            auto tmpbin = spectrumtmp->GetXaxis()->FindBin(spectrum->GetXaxis()->GetBinCenter(b+1));
            spectrum->SetBinContent(b+1, spectrumtmp->GetBinContent(tmpbin));
            spectrum->SetBinError(b+1, spectrumtmp->GetBinError(tmpbin));
        }
        result[r] = spectrum;
    }
    return result;
}

void makeComparisonUnfoldedRestricted(const std::string_view fileultra, const std::string_view filereg){
    auto dataUltra = readRawSpectra(fileultra), datareg = readRawSpectra(filereg);

    auto plot = new ROOT6tools::TSavableCanvas("comparisonUnfoldedRanges", "Comparison raw spectra in identical range", 300 * dataUltra.size(), 700);
    plot->Divide(dataUltra.size(),2);

    Style ultrastyle{kRed, 24}, regstyle{kBlue, 25}, ratiostyle{kBlack, 20};
    int icol = 0;
    for(auto ultrait = dataUltra.begin(), regit = datareg.begin(); ultrait != dataUltra.end(); ++ultrait, ++regit){
        plot->cd(icol+1);
        gPad->SetTopMargin(0.02);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.02);
        gPad->SetLogy();
        auto specframe = new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(ultrait->first * 10.)), "p_{t} (GeV/c)", "dN/dp_{t}", 0., 250, 1e-7, 1e-1);
        specframe->GetXaxis()->SetTitleSize(0.045);
        specframe->GetXaxis()->SetLabelSize(0.045);
        specframe->GetYaxis()->SetTitleSize(0.045);
        specframe->GetYaxis()->SetLabelSize(0.045);
        specframe->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.25, 0.15, 0.4, 0.22, Form("R=%.1f", ultrait->first)))->Draw();
        TLegend *leg(nullptr);
        if(!icol){
            leg = new ROOT6tools::TDefaultLegend(0.6, 0.7, 0.95, 0.95);
            leg->SetTextSize(0.045);
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
        gPad->SetTopMargin(0.02);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.02);
        auto ratioframe = new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(ultrait->first * 10.)), "p_{t} (GeV/c)", "wide/normal", 0., 250, 0.8, 1.2); 
        ratioframe->GetXaxis()->SetTitleSize(0.045);
        ratioframe->GetXaxis()->SetLabelSize(0.045);
        ratioframe->GetYaxis()->SetTitleSize(0.045);
        ratioframe->GetYaxis()->SetLabelSize(0.045);
        ratioframe->Draw("axis");
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