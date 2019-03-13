#include "../../../helpers/graphics.C"

std::map<double, TH1 *> readCorrectedSpectra(const std::string_view filename){
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(auto rdir : *gDirectory->GetListOfKeys()){
        std::string rstring(rdir->GetName());
        double rval = double(std::stoi(rstring.substr(1)))/10.;
        reader->cd(rstring.data());
        gDirectory->cd("reg4");
        TH1 *spectrum = static_cast<TH1 *>(gDirectory->Get("normalized_reg4"));
        spectrum->SetDirectory(nullptr);
        result[rval] = spectrum;
    }
    return result;
}

void ComparisonReconstructionScheme(const std::string_view basefile){
    std::map<double, TH1 *> spectraPtScheme = readCorrectedSpectra(Form("ptscheme/%s", basefile.data())),
                            spectraEScheme = readCorrectedSpectra(Form("Escheme/%s", basefile.data()));

    int nrad = spectraPtScheme.size();
    auto plot = new ROOT6tools::TSavableCanvas("comparisonRecombinationScheme", "Comparison recombination scheme", 300 * nrad, 700);
    plot->Divide(nrad, 2);

    Style ptstyle{kRed, 24}, Estyle{kBlue, 25};
    int icol = 0;
    for(auto ptit = spectraPtScheme.begin(), Eit = spectraEScheme.begin(); ptit != spectraPtScheme.end(); ++ptit, ++Eit){
        plot->cd(icol+1);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(ptit->first * 10.)), "p_{t} (GeV/c)", "d#sigma/dp_{t}dy (mb/(GeV/c))", 0., 240., 1e-10, 10.))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.3, 0.22, Form("R=%.1f", ptit->first)))->Draw();
        TLegend *leg(nullptr);
        if(!icol) {
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
            leg->Draw();
        }
        ptstyle.SetStyle<TH1>(*ptit->second);
        Estyle.SetStyle<TH1>(*Eit->second);
        ptit->second->Draw("epsame");
        Eit->second->Draw("epsame");
        if(leg) {
            leg->AddEntry(ptit->second, "p_{t}-scheme", "lep");
            leg->AddEntry(Eit->second, "E-scheme", "lep");
        }

        plot->cd(icol+nrad+1);
        (new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(ptit->first * 10.)), "p_{t} (GeV/c)", "p_{t}-scheme/E-scheme", 0., 240., 0.5, 1.5))->Draw("axis");
        auto ratio = static_cast<TH1 *>(ptit->second->Clone(Form("ratioschemes_R%02d", int(ptit->first * 10.))));
        ratio->SetDirectory(nullptr);
        ratio->Divide(Eit->second);
        Style{kBlack, 20}.SetStyle<TH1>(*ratio);
        ratio->Draw("epsame");
        icol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}