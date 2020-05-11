#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

void checkOutlierCutSD(const char *nameWithCut, const char *nameWithoutCut, const char *probe = "Zg") {

    std::unique_ptr<TFile> readerWithCut(TFile::Open(nameWithCut, "READ")),
                           readerWithoutCut(TFile::Open(nameWithoutCut, "READ"));

    auto style = [](Color_t col, Style_t mrk) {
        return [col, mrk] (TH1 *hist) {
            hist->SetMarkerColor(col);
            hist->SetMarkerStyle(mrk);
            hist->SetLineColor(col);
        };
    };

    const double ptmin = 30., ptmax = 200.;
    const int defaultiteration = 6;
    readerWithCut->cd(probe);
    auto basedirWithCut = gDirectory;
    readerWithoutCut->cd(probe);
    auto basedirWithoutCut = gDirectory;
    auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.89, 0.89);
    for(auto R : ROOT::TSeqI(2, 7)){
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        basedirWithCut->cd(rstring.data());
        gDirectory->cd(Form("Iter%d", defaultiteration));
        auto correctedhistWithCut = static_cast<TH2 *>(gDirectory->Get(Form("correctedIter%d_%s_%s", defaultiteration, probe, rstring.data())));
        basedirWithoutCut->cd(rstring.data());
        gDirectory->cd(Form("Iter%d", defaultiteration));
        auto correctedhistWithoutCut = static_cast<TH2 *>(gDirectory->Get(Form("correctedIter%d_%s_%s", defaultiteration, probe, rstring.data())));

        int currentcol = 0, currentplot = 0;
        ROOT6tools::TSavableCanvas *plot(nullptr);
        for(auto iptb : ROOT::TSeqI(0, correctedhistWithCut->GetYaxis()->GetNbins())) {
            auto ptlow = correctedhistWithCut->GetYaxis()->GetBinLowEdge(iptb+1),
                 pthigh = correctedhistWithCut->GetYaxis()->GetBinUpEdge(iptb+1),
                 ptcent = correctedhistWithCut->GetYaxis()->GetBinCenter(iptb+1);
            if(ptcent < ptmin || ptcent > ptmax) continue;
            std::cout << "Doing bin from " << ptlow << " GeV/c to " << pthigh << " GeV/c" << std::endl;
            if(currentcol % 5 == 0){
                if(plot) {
                    plot->cd();
                    plot->Update();
                    plot->SaveCanvas(plot->GetName());
                }
                plot = new ROOT6tools::TSavableCanvas(Form("compartisonOutlierCut%s%s_%d", probe, rstring.data(), currentplot), Form("Comparison outlier cut of %s (%d)", probe, currentplot), 1200, 600);
                plot->Divide(5,2);
                currentcol=0;
                currentplot++;
            }
            plot->cd(currentcol+1);

            (new ROOT6tools::TAxisFrame(Form("%sdistframe%s_%d_%d", probe, rstring.data(), int(ptlow), int(pthigh)), probe, Form("1/N_{jets} dN/d%s", probe), correctedhistWithCut->GetXaxis()->GetBinLowEdge(1), correctedhistWithCut->GetXaxis()->GetBinUpEdge(correctedhistWithCut->GetXaxis()->GetNbins()), 0., 10.))->Draw("axis");
            (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.89, 0.89, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptlow, pthigh)))->Draw();
            if(currentcol == 0) {
                leg = new ROOT6tools::TDefaultLegend(0.5, 0.5, 0.89, 0.79);
                leg->Draw();
                (new ROOT6tools::TNDCLabel(0.15, 0.7, 0.35, 0.79, rtitle.data()))->Draw(); 
            }

            auto projectedWithCut = correctedhistWithCut->ProjectionX(Form("projectionWithCut%s_%s_%d_%d", probe, rstring.data(), int(ptlow), int(pthigh)), iptb+1, iptb+1);
            std::cout << "Got projection with name " << projectedWithCut->GetName() << std::endl;
            projectedWithCut->SetDirectory(nullptr);
            projectedWithCut->Scale(1./projectedWithCut->Integral());
            projectedWithCut->Scale(1., "width"); 
            style(kRed,24)(projectedWithCut);
            projectedWithCut->Draw("epsame");
 
            auto projectedWithoutCut = correctedhistWithoutCut->ProjectionX(Form("projectionWithoutCut%s_%s_%d_%d", probe, rstring.data(), int(ptlow), int(pthigh)), iptb+1, iptb+1);
            std::cout << "Got projection cut with name " << projectedWithoutCut->GetName() << std::endl;
            projectedWithoutCut->SetDirectory(nullptr);
            projectedWithoutCut->Scale(1./projectedWithoutCut->Integral());
            projectedWithoutCut->Scale(1., "width"); 
            style(kBlue,25)(projectedWithoutCut);
            projectedWithoutCut->Draw("epsame");

            if(currentcol == 0) {
                leg->AddEntry(projectedWithCut, "With outlier cut", "lep");
                leg->AddEntry(projectedWithoutCut, "No outlier cut", "lep");
            } 

            plot->cd(currentcol+5+1);
            (new ROOT6tools::TAxisFrame(Form("%sratioframe%s_%d_%d", probe, rstring.data(), int(ptlow), int(pthigh)), probe, "With/without outlier cut", correctedhistWithCut->GetXaxis()->GetBinLowEdge(1), correctedhistWithCut->GetXaxis()->GetBinUpEdge(correctedhistWithCut->GetXaxis()->GetNbins()), 0., 2.))->Draw("axis");
            auto ratio = static_cast<TH1 *>(projectedWithCut->Clone(Form("ratioCut_%s_%s_%d_%d", probe, rstring.data(), int(ptlow), int(pthigh))));
            ratio->SetDirectory(nullptr);
            ratio->Divide(projectedWithoutCut);
            style(kBlack, 20)(ratio);
            ratio->Draw("epsame");
            currentcol++;
        }
        if(plot){
            plot->cd();
            plot->Update();
            plot->SaveCanvas(plot->GetName());
        }
    }
}