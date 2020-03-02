#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../binnings/binningPt1D.C"

std::vector<std::string> emcaltriggers = {"EJ1", "EJ2"};
std::map<std::string, int> triggerbin = {{"EJ1", 3}, {"EJ2", 1}};
std::map<std::string, double> defaultenergies = {{"EJ1", 19.}, {"EJ2", 14.}};

void compareCorrectedTriggerEfficiency(const char *datafile = "data_ref/AnalysisResults.root", const char *trgefffile = "TriggerEfficiencies.root"){

    std::vector<Color_t> colors = {kRed, kGreen, kBlue, kViolet, kTeal, kOrange, kMagenta, kGray, kAzure};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32};

    std::unique_ptr<TFile> datareader(TFile::Open(datafile, "READ")),
                           trgreader(TFile::Open(trgefffile, "READ"));

    std::map<std::string, ROOT6tools::TSavableCanvas *> trgcanvas;
    for(auto trg : emcaltriggers) {
        auto plot = new ROOT6tools::TSavableCanvas(Form("compSpecCorrEff%s", trg.data()), Form("Comparison spectra after correction for the trigger efficiency %s", trg.data()), 1200, 600);
        plot->Divide(5,2);
        trgcanvas[trg] = plot;
    } 

    auto style = [](Color_t col, Style_t marker) {
        return [col, marker] (auto obj) {
            obj->SetMarkerColor(col);
            obj->SetMarkerStyle(marker);
            obj->SetLineColor(col);
        };
    };
    
    auto plotptbinning = getJetPtBinningNonLinSmearPoor();

    int icol = 0; 
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", R));

        // get min. bias spectrum
        datareader->cd(Form("JetSpectrum_FullJets_%s_INT7_tc200", rstring.data()));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto hint2d = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
        auto mbrefIn = hint2d->ProjectionY(Form("mbrefIn_%s", rstring.data()), 1, 1);   // take trigger ANY for min. bias
        auto mbref = mbrefIn->Rebin(plotptbinning.size()-1, Form("mbrefIn_%s", rstring.data()), plotptbinning.data());
        mbref->SetDirectory(nullptr);
        auto hnorm = static_cast<TH1 *>(histlist->FindObject("hClusterCounter"));
        auto normMB = hnorm->GetBinContent(1);
        mbref->Scale(1./normMB);
        mbref->Scale(1, "width");
        style(kBlack, 20)(mbref);

        for(auto trg : emcaltriggers){
            auto plot = trgcanvas[trg];
            TLegend *leg(nullptr);
            plot->cd(icol+1);
            gPad->SetLogy();
            (new ROOT6tools::TAxisFrame(Form("FrameEff%s", rstring.data()), "p_{t,jet} (GeV/c)", "1/N_{ev} dN/dp_{t} ((GeV/c)^{-1})", 0., 200., 1e-11, 1e-1))->Draw("axis");
            (new ROOT6tools::TNDCLabel(0.6, 0.15, 0.89, 0.24, Form("R=%.1f", double(R)/10.)))->Draw();
            if(!icol) {
                leg = new ROOT6tools::TDefaultLegend(0.15, 0.65, 0.89, 0.89);
                leg->SetNColumns(2);
                leg->Draw();
            }
            plot->cd(icol+6);
            (new ROOT6tools::TAxisFrame(Form("FrameEff%s", rstring.data()), "p_{t,jet} (GeV/c)", "Trig / Min. Bias", 0., 200., 0., 1.5))->Draw("axis");

            datareader->cd(Form("JetSpectrum_FullJets_%s_%s_tc200", rstring.data(), trg.data()));
            histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
            auto htrg2d = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
            auto trgbin = triggerbin[trg];
            auto trgspec = htrg2d->ProjectionY(Form("mbref_%s", rstring.data()), trgbin, trgbin);   // take trigger ANY for min. bias
            trgspec->SetDirectory(nullptr);
            if(trg == "EJ1"){
                // Apply CENTNOTRD correction
                hnorm = static_cast<TH1 *>(histlist->FindObject("hClusterCounterAbs"));
                auto centnotrdcorrection = hnorm->GetBinContent(3)/hnorm->GetBinContent(2);
                trgspec->Scale(1./centnotrdcorrection);
            }
            trgspec->Scale(1./normMB);

            plot->cd(icol+1);
            mbref->Draw("epsame");
            if(leg) leg->AddEntry(mbref, "INT7", "lep");

            // apply variations
            trgreader->cd(trg.data());
            gDirectory->cd(rstring.data());
            int ivar = 0;
            for(auto vareff : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
                std::string effstring(vareff->GetName());
                std::string varstring = effstring.substr(26);
                double energy = defaultenergies[trg];
                if (varstring != "default") energy = double(std::stoi(varstring))/10.;
                auto effhist = vareff->ReadObject<TH1>();

                auto spectrumCorrectedIn = static_cast<TH1 *>(trgspec->Clone(Form("trgSpecCorrectedIn_%s_%s_%s", trg.data(), rstring.data(), varstring.data())));
                spectrumCorrectedIn->Divide(effhist);
                auto spectrumCorrected = spectrumCorrectedIn->Rebin(plotptbinning.size() - 1, Form("trgSpecCorrected_%s_%s_%s", trg.data(), rstring.data(), varstring.data()), plotptbinning.data());
                spectrumCorrected->SetDirectory(nullptr);
                spectrumCorrected->Scale(1., "width");
                style(colors[ivar], markers[ivar])(spectrumCorrected);

                plot->cd(icol+1);
                spectrumCorrected->Draw("epsame");
                if(leg) {
                    std::string legtitle = Form("%.1f GeV", energy);
                    if(varstring == "default") legtitle += " (default)";
                    leg->AddEntry(spectrumCorrected, legtitle.data(), "lep");
                }

                plot->cd(icol + 6);
                auto ratio = static_cast<TH1 *>(spectrumCorrected->Clone(Form("ratio_%s_min_bias_%s_%s", trg.data(), rstring.data(), varstring.data())));
                ratio->SetDirectory(nullptr);
                ratio->Divide(mbref);
                ratio->Draw("epsame");
                ivar++;
            }
        }
        icol++;
    }

    for(auto [trg, plot] : trgcanvas) {
        plot->cd();
        plot->Update();
        plot->SaveCanvas(plot->GetName());
    }
}