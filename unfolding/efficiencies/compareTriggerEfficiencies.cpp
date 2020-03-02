#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

std::map<std::string, double> ej1variation = {{"default", 19.}, {"v1", 17.}, {"v2", 17.5}, {"v3", 18.}, {"v4", 18.5}, {"v5", 19.5}, {"v6", 20.}, {"v7", 20.5}, {"v8", 21.}},
                              ej2variation = {{"default", 14.}, {"v1", 12.}, {"v2", 12.5}, {"v3", 13.}, {"v4", 13.5}, {"v5", 14.5}, {"v6", 15.}, {"v7", 15.5}, {"v8", 16.}};
std::map<std::string, std::map<std::string, double>> variations = {{"EJ1", ej1variation}, {"EJ2", ej2variation}};
std::vector<std::string> emcaltriggers = {"EJ1", "EJ2"};
std::map<std::string, Color_t> colors = {{"v1", kRed}, {"v2", kGreen}, {"v3", kBlue}, {"v4", kViolet}, {"v5", kTeal}, {"v6", kOrange}, {"v7", kMagenta}, {"v8", kGray}};
std::map<std::string, Style_t> markers = {{"v1", 24}, {"v2", 25}, {"v3", 26}, {"v4", 27}, {"v5", 28}, {"v6", 29}, {"v7", 30}, {"v8", 31}};

void compareTriggerEfficiencies(const char *filename = "AnalysisResults.root"){
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ")),
                           writer(TFile::Open("TriggerEfficiencies.root", "RECREATE"));
    writer->mkdir("EJ1");
    writer->mkdir("EJ2");

    std::map<std::string, ROOT6tools::TSavableCanvas *> trgcanvas;
    for(auto trg : emcaltriggers) {
        auto plot = new ROOT6tools::TSavableCanvas(Form("compTrgEff%s", trg.data()), Form("Comparison trigger efficiency %s", trg.data()), 1200, 600);
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

    int icol = 0;
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", R));

        // get min. bias spectrum
        reader->cd(Form("JetSpectrum_FullJets_%s_INT7_tc200", rstring.data()));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto hint2d = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
        auto mbref = hint2d->ProjectionY(Form("mbref_%s", rstring.data()), 1, 1);
        mbref->SetDirectory(nullptr);

        for(auto trg : emcaltriggers) {
            auto plot = trgcanvas[trg];
            auto trgvar = variations[trg];
            TLegend *leg(nullptr);
            plot->cd(icol+1);
            (new ROOT6tools::TAxisFrame(Form("FrameEff%s", rstring.data()), "p_{t,jet} (GeV/c)", "Trigger efficiency", 0., 200., 0., 1.5))->Draw("axis");
            (new ROOT6tools::TNDCLabel(0.6, 0.15, 0.89, 0.24, Form("R=%.1f", double(R)/10.)))->Draw();
            if(!icol) {
                leg = new ROOT6tools::TDefaultLegend(0.15, 0.65, 0.89, 0.89);
                leg->SetNColumns(2);
                leg->Draw();
            }
            plot->cd(icol+6);
            (new ROOT6tools::TAxisFrame(Form("FrameEff%s", rstring.data()), "p_{t,jet} (GeV/c)", "Variation / Default", 0., 200., 0., 1.5))->Draw("axis");
            plot->cd(icol+1);

            // get default spectrum 
            reader->cd(Form("JetSpectrum_FullJets_%s_%s_Default", rstring.data(), trg.data()));
            histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
            auto hdef2d = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
            auto effdefault = hdef2d->ProjectionY(Form("effdefault_%s_%s", rstring.data(), trg.data()), 1, 1);
            effdefault->SetDirectory(nullptr);
            effdefault->Divide(effdefault, mbref, 1., 1., "b");
            style(kBlack, 20)(effdefault);
            effdefault->Draw("epsame");
            if(leg) leg->AddEntry(effdefault, Form("Default: %.1f GeV", trgvar["default"]), "lep");

            std::map<std::string, TH1 *> outputEfficiencies = {{"default", effdefault}};

            for (auto var : ROOT::TSeqI(1, 9)) {
                std::string varstring(Form("v%d", var));

                reader->cd(Form("JetSpectrum_FullJets_%s_%s_%s", rstring.data(), trg.data(), varstring.data()));
                histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
                auto hvar2d = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
                auto effvar = hvar2d->ProjectionY(Form("effvar_%s_%s_%s", rstring.data(), trg.data(), varstring.data()), 1, 1);
                effvar->SetDirectory(nullptr);
                effvar->Divide(effvar, mbref, 1., 1., "b");
                style(colors[varstring], markers[varstring])(effvar);
                plot->cd(icol+1);
                effvar->Draw("epsame");
                if(leg) leg->AddEntry(effvar, Form("%s: %.1f GeV", varstring.data(), trgvar[varstring]), "lep");

                plot->cd(icol+6);
                auto ratioDefault = static_cast<TH1 *>(effvar->Clone(Form("Ratio_%s_%s", effvar->GetName(), effdefault->GetName())));
                ratioDefault->SetDirectory(nullptr);
                ratioDefault->Divide(effdefault);
                ratioDefault->Draw("epsame");

                std::string tag(Form("%d", int(trgvar[varstring] * 10.)));
                outputEfficiencies[tag] = effvar; 
            }

            writer->cd(trg.data());
            gDirectory->mkdir(rstring.data());
            gDirectory->cd(rstring.data());
            for(auto [tag, eff] : outputEfficiencies) {
                eff->SetName(Form("TriggerEfficiency_%s_%s_%s", trg.data(), rstring.data(), tag.data()));
                eff->Write();
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