#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"
#include "../../../helpers/root.C"
#include "../../../helpers/string.C"


void ComparisonFoldRaw_SpectrumTask(const std::string_view inputfile){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    auto nradius = reader->GetListOfKeys()->GetEntries();

    bool isSVD = (inputfile.find("Svd") != std::string::npos);

    auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonFoldRaw%s", (isSVD ? "Svd" : "Bayes")), Form("Comparison back-folded raw (%s unfolding)", (isSVD ? "SVD" : "Bayes")), nradius * 300., 700.);
    plot->Divide(nradius, 2);

    std::array<Color_t, 10> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kMagenta, kGray, kAzure, kCyan};
    std::array<Style_t, 10> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

    int currentcol = 0;
    for(auto k : *reader->GetListOfKeys()){
        std::string rstring(k->GetName());
        double rvalue = double(std::stoi(rstring.substr(1, 2)))/10.;
        
        reader->cd(k->GetName());
        auto basedir = gDirectory;
        
        // get the raw spectrum
        gDirectory->cd("rawlevel");
        auto rawspectrum = static_cast<TH1 *>(gDirectory->Get(Form("hraw_%s", rstring.data())));
        rawspectrum->SetDirectory(nullptr);
        normalizeBinWidth(rawspectrum);
        Style{kBlack, 20}.SetStyle<TH1>(*rawspectrum);

        plot->cd(1+currentcol);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("specframe_%s", rstring.data()), "p_{t} (GeV/c)", "1/N_{ev} dN/dp_{t} ((GeV/c)^{-1})", 0., 200., 1e-8, 1e-3))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("R = %.1f", rvalue)))->Draw();
        TLegend *leg(nullptr);
        if(!currentcol) {
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.45, 0.89, 0.89);
            leg->Draw();
        }
        rawspectrum->Draw("epsame");
        if(leg) leg->AddEntry(rawspectrum, "raw", "lep");

        plot->cd(1 + currentcol + nradius);
        (new ROOT6tools::TAxisFrame(Form("ratioframe_%s", rstring.data()), "p_{t} (GeV/c)", "Folded/raw", 0., 200., 0.5, 1.5))->Draw("axis");   

        for(auto ireg : ROOT::TSeqI(1, 11)){
            basedir->cd(Form("reg%d", ireg));
            auto backfolded = static_cast<TH1 *>(gDirectory->Get(Form("backfolded_reg%d", ireg)));
            backfolded->SetDirectory(nullptr);
            normalizeBinWidth(backfolded);
            Style{colors[ireg-1], markers[ireg-1]}.SetStyle<TH1>(*backfolded);
            plot->cd(1+currentcol);
            backfolded->Draw("epsame");
            if(leg) leg->AddEntry(backfolded, Form("reg=%d", ireg), "lep");
            plot->cd(1+currentcol+nradius);
            auto ratio = static_cast<TH1*>(backfolded->Clone(Form("ratioFoldRaw_%s_reg%d", rstring.data(), ireg)));
            ratio->SetDirectory(nullptr);
            ratio->Divide(rawspectrum);
            ratio->Draw("epsame");
        }
        currentcol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}