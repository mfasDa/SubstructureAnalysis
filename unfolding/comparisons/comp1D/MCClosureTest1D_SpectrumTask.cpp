#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"
#include "../../../helpers/root.C"
#include "../../../helpers/string.C"

void MCClosureTest1D_SpectrumTask(const std::string_view inputfile) {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    auto nradius = reader->GetListOfKeys()->GetEntries();

    auto plot = new ROOT6tools::TSavableCanvas("MCClosureTest1D", "Monte-Calro closure test", nradius * 300., 700.);
    plot->Divide(nradius, 2);

    std::array<Color_t, 10> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kMagenta, kGray, kAzure, kCyan};
    std::array<Style_t, 10> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};

    int currentcol = 0;
    for(auto k : *reader->GetListOfKeys()){
        std::string rstring(k->GetName());
        double rvalue = double(std::stoi(rstring.substr(1, 2)))/10.;
        
        reader->cd(k->GetName());
        auto basedir = gDirectory;
        
        // get truth level spectrum
        basedir->cd("closuretest");
        auto *htruth = static_cast<TH1 *>(gDirectory->Get("partclosure"));
        htruth->SetDirectory(nullptr);
        normalizeBinWidth(htruth);
        Style{kBlack, 20}.SetStyle<TH1>(*htruth);

        plot->cd(1+currentcol);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("specframe_%s", rstring.data()), "p_{t} (GeV/c)", "d#sigma/dp_{t} (mb/(GeV/c))", 0., 200., 1e-8, 10))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("R = %.1f", rvalue)))->Draw();
        TLegend *leg(nullptr);
        if(!currentcol) {
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.45, 0.89, 0.89);
            leg->Draw();
        }
        htruth->Draw("epsame");
        if(leg) leg->AddEntry(htruth, "raw", "lep");

        plot->cd(1 + currentcol + nradius);
        (new ROOT6tools::TAxisFrame(Form("ratioframe_%s", rstring.data()), "p_{t} (GeV/c)", "Unfolded/true", 0., 200., 0.5, 1.5))->Draw("axis");   

        for(auto ireg : ROOT::TSeqI(1, 11)){
            basedir->cd(Form("reg%d", ireg));
            auto unfolded = static_cast<TH1 *>(gDirectory->Get(Form("unfoldedClosure_reg%d", ireg)));
            unfolded->SetDirectory(nullptr);
            normalizeBinWidth(unfolded);
            Style{colors[ireg-1], markers[ireg-1]}.SetStyle<TH1>(*unfolded);
            plot->cd(1+currentcol);
            unfolded->Draw("epsame");
            if(leg) leg->AddEntry(unfolded, Form("reg=%d", ireg), "lep");
            plot->cd(1+currentcol+nradius);
            auto ratio = static_cast<TH1*>(unfolded->Clone(Form("ratioUnfoldedTrue_%s_reg%d", rstring.data(), ireg)));
            ratio->SetDirectory(nullptr);
            ratio->Divide(htruth);
            ratio->Draw("epsame");
        }
        currentcol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}