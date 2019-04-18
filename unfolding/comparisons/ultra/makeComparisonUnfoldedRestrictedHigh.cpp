#include "../../binnings/binningPt1D.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"

std::map<double, TH1 *> readRawSpectra(const std::string_view filename){
    auto binningtmp = getJetPtBinningNonLinTrueUltra240();
    std::vector<double> binning;
    // restrict to reported range of the measurement with the regular binning
    for(auto b : binningtmp) {
        if(b < 20.) continue;
        if(b > 321.) break;
        binning.push_back(b);
    }
    TH1D histtemplate("histtemplate", "histtemplate", binning.size()-1, binning.data());
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(double r = 0.2; r <= 0.6; r+= 0.1) {
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

void makeComparisonUnfoldedRestrictedHigh(const std::string_view sysvar = "notc"){
    auto data300 = readRawSpectra(Form("correctedSVD_ultra300_%s.root", sysvar.data())), data240 = readRawSpectra(Form("correctedSVD_ultra240_%s.root", sysvar.data()));

    auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonUnfoldedRangesHigh%s", sysvar.data()), "Comparison raw spectra in identical range", 300 * data300.size(), 700);
    plot->Divide(data300.size(),2);

    Style style300{kRed, 24}, style240{kBlue, 25}, ratiostyle{kBlack, 20};
    int icol = 0;
    for(auto it300 = data300.begin(), it240 = data240.begin(); it300 != data300.end(); ++it300, ++it240){
        plot->cd(icol+1);
        gPad->SetTopMargin(0.02);
        gPad->SetLeftMargin(0.16);
        gPad->SetRightMargin(0.02);
        gPad->SetLogy();
        auto specframe = new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(it300->first * 10.)), "p_{t} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., 350, 1e-9, 1e-1);
        specframe->GetXaxis()->SetTitleSize(0.045);
        specframe->GetXaxis()->SetLabelSize(0.045);
        specframe->GetYaxis()->SetTitleSize(0.045);
        specframe->GetYaxis()->SetLabelSize(0.045);
        specframe->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.25, 0.15, 0.4, 0.22, Form("R=%.1f", it300->first)))->Draw();
        TLegend *leg(nullptr);
        if(!icol){
            leg = new ROOT6tools::TDefaultLegend(0.4, 0.7, 0.95, 0.95);
            leg->SetTextSize(0.045);
            leg->Draw();
        }

        style300.SetStyle<TH1>(*it300->second);
        style240.SetStyle<TH1>(*it240->second);
        it300->second->Draw("epsame");
        it240->second->Draw("epsame");
        if(leg) {
            leg->AddEntry(it240->second, "p_{t,det} < 240 GeV/c", "lep" );
            leg->AddEntry(it300->second, "p_{t,det} < 300 GeV/c", "lep" );
        }
        plot->cd(icol+data300.size()+1);
        gPad->SetTopMargin(0.02);
        gPad->SetLeftMargin(0.16);
        gPad->SetRightMargin(0.02);
        auto ratioframe = new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(it300->first * 10.)), "p_{t} (GeV/c)", "(p_{t,det} < 300 GeV/c)/(p_{t,det} < 240 GeV/c)", 0., 350, 0.8, 1.2); 
        ratioframe->GetXaxis()->SetTitleSize(0.045);
        ratioframe->GetXaxis()->SetLabelSize(0.045);
        ratioframe->GetYaxis()->SetTitleSize(0.045);
        ratioframe->GetYaxis()->SetLabelSize(0.045);
        ratioframe->Draw("axis");
        auto ratio = static_cast<TH1 *>(it300->second->Clone(Form("ratioR%02d", int(it300->first * 10.))));
        ratio->SetDirectory(nullptr);
        ratio->Divide(it240->second);
        ratiostyle.SetStyle<TH1>(*ratio);
        ratio->Draw("epsame");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}