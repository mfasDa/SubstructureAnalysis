#ifndef __CLING__
#include <array>
#include <iostream>
#include <memory>

#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH2.h>
#endif

template<typename t>
t *Get(TFile *reader, const char *key){
    t *result(nullptr);
    result = static_cast<t *>(reader->Get(key));
    return result;
}

template<typename t>
std::unique_ptr<t> make_unique(t *object){
    std::unique_ptr<t> ptr(object);
    return ptr;
}

void makeFoldingComparisonZg(std::string_view inputfile) {
    const std::array<int, 8> ITERATIONS = {{1, 4, 10, 15, 20, 25, 30, 34}};

    auto plot = new TCanvas("foldingComparison", "Folding comparsion", 1000, 600);
    plot->Divide(4,2);

    auto filereader = make_unique<TFile>(TFile::Open(inputfile.data(),"READ"));
    auto hraw = Get<TH2>(filereader.get(), "hraw");

    int ipad(1);
    for(auto niter : ITERATIONS){
        auto folded = Get<TH2>(filereader.get(), Form("zg_folded_iter%d.root", niter));
        if(!folded) std::cerr << "Folded histogram not found for iter" << niter << std::endl;
        auto foldedoverraw = static_cast<TH2 *>(folded->Clone(Form("foldedoverraw_iter%d", niter)));
        foldedoverraw->Divide(hraw);
        foldedoverraw->SetStats(false);
        foldedoverraw->SetDirectory(nullptr);
        foldedoverraw->SetTitle("");
        foldedoverraw->GetXaxis()->SetTitle("z_{g}");
        foldedoverraw->GetYaxis()->SetTitle("p_{t,smear} (GeV/c)");

        plot->cd(ipad++);
        gPad->SetLeftMargin(0.13);
        foldedoverraw->Draw("colz");
        auto iterlabel = new TPaveText(0.6, 0.90, 0.89, 0.98, "NDC");
        iterlabel->SetBorderSize(0);
        iterlabel->SetFillStyle(0);
        iterlabel->SetTextFont(42);
        iterlabel->AddText(Form("niter=%d", niter));
        iterlabel->Draw();
    }

    plot->cd();
}