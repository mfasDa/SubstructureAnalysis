#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

void makeComparisonNormalizedProjection(const char *inputfile = "UnfoldedSD.root", const char *observable = "Zg"){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ"));
    reader->cd(observable);
    auto basedir = gDirectory;
    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2", "combined"};

    auto style = [](Color_t col, Style_t mrk) {
        return [col, mrk](auto obj) {
            obj->SetMarkerColor(col);
            obj->SetLineColor(col);
            obj->SetMarkerStyle(mrk);
        };
    };

    std::map<std::string, Color_t> colors = {{"INT7", kBlack}, {"EJ1", kRed}, {"EJ2", kBlue}, {"combined", kViolet}};
    std::map<std::string, Style_t> markers = {{"INT7", 20}, {"EJ1", 24}, {"EJ2", 25}};

    auto plot = new ROOT6tools::TSavableCanvas(Form("ComparisonPtProjections1D_%s", observable), Form("Comparison trigger pt-projections for %s", observable), 1200, 800);
    plot->Divide(5,2);

    int icol = 0; 
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", R)), rtitle(Form("R = %.1f", double(R)/10.));
        basedir->cd(rstring.data());
        gDirectory->cd("rawlevel");
        std::map<std::string, TH1 *> projections;
        for(const auto &trg : triggers) {
            auto hist2D = static_cast<TH2 *>(gDirectory->Get(Form("rawlevel%s_%s_%s", observable, rstring.data(), trg.data())));
            auto projection1D = hist2D->ProjectionY(Form("specPt_%s_%s", rstring.data(), trg.data()));
            projection1D->SetDirectory(nullptr);
            projection1D->Scale(1, "width");
            projections[trg] = projection1D;
        }

        plot->cd(icol+1);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("axisSpec_%s", rstring.data()), "p_{t}", "dN/dp_{t}", 0, 240, 1, 1e9))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.4, 0.22, rtitle.data()));
        TLegend *leg = nullptr;
        if(icol == 0) {
            leg = new ROOT6tools::TDefaultLegend(0.5, 0.65, 0.89, 0.89);
            leg->Draw();
        }

        for(auto [trg, spec] : projections) {
            style(colors[trg], markers[trg])(spec);
            spec->Draw("epsame");
            if(leg) leg->AddEntry(spec, trg.data(), "lep");
        }

        plot->cd(icol + 6);
        (new ROOT6tools::TAxisFrame(Form("axisRatio_%s", rstring.data()), "p_{t}", "Trigger / INT7", 0, 240, 0, 2))->Draw("axis");
        auto reference = projections["INT7"];
        for(auto [trg, spec] : projections) {
            if(trg == "INT7") continue;
            auto ratio = static_cast<TH1 *>(spec->Clone(Form("Ratio_%s_INT7_%s", trg.data(), rstring.data())));
            ratio->SetDirectory(nullptr);
            ratio->Divide(reference);
            style(colors[trg], markers[trg])(ratio);
            ratio->Draw("epsame");
        }
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}