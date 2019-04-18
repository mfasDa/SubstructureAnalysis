#include "../../binnings/binningPt1D.C"
#include "../../../helpers/graphics.C"

struct variation {
    int         fTCcut;
    Style       fStyle;

    bool operator<(const variation &other) const { return fTCcut < other.fTCcut; }
};

std::map<double, TH1 *> readRawSpectra(const std::string_view filename, int ultrarange){
    double ptmaxreport;
    std::vector<double> binningtmp;
    if(ultrarange == 240) {
        ptmaxreport = 321.;
        binningtmp = getJetPtBinningNonLinTrueUltra240();
    } else if (ultrarange == 300){
        ptmaxreport = 401.;
        binningtmp = getJetPtBinningNonLinTrueUltra300();
    }
    std::vector<double> binning;
    // restrict to reported range of the measurement with the regular binning
    for(auto b : binningtmp) {
        if(b < 20.) continue;
        if(b > ptmaxreport) break;
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

std::vector<variation> getVariations(int ultrarange) {
    std::vector<variation> result;
    result.push_back({250, {kBlue, 25}});
    if(ultrarange == 300)result.push_back({1000, {kRed, 24}});
    return result;
}

void makeComparisonUnfoldedTruncationConstituents(int ultrarange) {
    std::map<double, TH1 *> reference = readRawSpectra(Form("correctedSVD_ultra%d_tc200.root", ultrarange), ultrarange);
    std::map<variation, std::map<double, TH1 *>> variations;
    for(auto v : getVariations(ultrarange)) {
        std::stringstream filename;
        filename << "correctedSVD_ultra" << ultrarange;
        if(v.fTCcut < 1000) filename << "_tc" << v.fTCcut;
        else filename << "_notc";
        filename << ".root";
        variations[v] = readRawSpectra(filename.str().data(), ultrarange);
    }

    double maxy;
    if(ultrarange == 240) maxy = 350;
    else if (ultrarange == 300) maxy = 450;
    Style refstyle{kBlack, 20};

    auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonTruncationClusterTrack_unfolding%d", ultrarange), Form("Comparison unfolded spectra with different cluster / track cuts"), 300 * reference.size(), 700);
    plot->Divide(reference.size(), 2);
    int icol = 0;
    for(double r = 0.2; r <= 0.6; r += 0.1) {
        plot->cd(icol+1);
        gPad->SetLogy();
        gPad->SetLeftMargin(0.17);
        gPad->SetRightMargin(0.04);
        gPad->SetTopMargin(0.04);
        std::string rstring = Form("R%02d", int(r*10.));
        auto specframe = new ROOT6tools::TAxisFrame(Form("specframe%s", rstring.data()), "p_{t,jet} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., maxy, 1e-9, 1.);
        specframe->GetXaxis()->SetTitleSize(0.05);
        specframe->GetYaxis()->SetTitleSize(0.05);
        specframe->GetXaxis()->SetLabelSize(0.047);
        specframe->GetYaxis()->SetLabelSize(0.047);
        specframe->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.22, 0.15, 0.47, 0.22, Form("R < %.1f", r)))->Draw();
        TLegend *leg(nullptr);
        if(!icol) {
            (new ROOT6tools::TNDCLabel(0.25, 0.84, 0.94, 0.94, Form("10 GeV/c < p_{t,det} < %d GeV/c", ultrarange)))->Draw();
            leg = new ROOT6tools::TDefaultLegend(0.45, 0.5, 0.89, 0.89);
            leg->Draw();
        }
        auto refspec = reference.find(r)->second;
        refstyle.SetStyle<TH1>(*refspec);
        refspec->Draw("epsame");
        if(leg) leg->AddEntry(refspec, "p_{t}^{t,c} < 200 GeV/c", "lep");
        std::vector<TH1 *> ratios;
        for(auto v : variations) {
            auto varspec = v.second.find(r)->second;
            v.first.fStyle.SetStyle<TH1>(*varspec);
            varspec->Draw("epsame");
            if(leg) {
                std::stringstream legentry;
                legentry << "p_{t}^{t,c} < ";
                if(v.first.fTCcut == 1000) legentry << ultrarange;
                else legentry << v.first.fTCcut;
                legentry << " GeV/c";
                leg->AddEntry(varspec, legentry.str().data(), "lep");
            }

            auto ratio = static_cast<TH1 *>(varspec->Clone(Form("ratio%s_%d_200", rstring.data(), v.first.fTCcut)));
            ratio->SetDirectory(nullptr);
            ratio->Divide(refspec);
            v.first.fStyle.SetStyle<TH1>(*ratio);
            ratios.emplace_back(ratio);
        }

        plot->cd(icol+1+reference.size());        
        gPad->SetLeftMargin(0.17);
        gPad->SetRightMargin(0.04);
        gPad->SetTopMargin(0.04);
        auto ratioframe = new ROOT6tools::TAxisFrame(Form("ratioframe%s", rstring.data()), "p_{t,jet} (GeV/c)", "variation / p_{t}^{t,c} < 200 GeV/c", 0., maxy, 0.7, 1.3);
        ratioframe->GetXaxis()->SetTitleSize(0.05);
        ratioframe->GetYaxis()->SetTitleSize(0.05);
        ratioframe->GetXaxis()->SetLabelSize(0.047);
        ratioframe->GetYaxis()->SetLabelSize(0.047);
        ratioframe->Draw("axis");
        for(auto r : ratios) r->Draw("epsame");
        icol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}