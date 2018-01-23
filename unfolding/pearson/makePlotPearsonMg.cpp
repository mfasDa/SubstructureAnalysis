#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH2.h>
#include <TPaveText.h>
#endif

struct PtBin {
    double fMin;
    double fMax;

    bool operator==(const PtBin &other) const {
        return (fMin == other.fMin) && (fMax == other.fMax);
    }

    bool operator<(const PtBin &other) const {
        return fMax <= other.fMin;
    }
};

struct Range {
    double fMin;
    double fMax;
};

std::vector<PtBin> ExtractPtBinning(TFile &reader) {
    auto hist = static_cast<TH2 *>(reader.Get("mass_unfolded_iter4"));
    std::vector<PtBin> bins;
    for(auto b : ROOT::TSeqI(1, hist->GetYaxis()->GetNbins()+1)) bins.push_back({hist->GetYaxis()->GetBinLowEdge(b), hist->GetYaxis()->GetBinUpEdge(b)});
    std::sort(bins.begin(), bins.end(), std::less<PtBin>());
    return bins;
} 

Range GetRangeForTrigger(std::string_view filename) {
    Range result = {0., 0.};
    if(filename.find("INT7") != std::string::npos){
       result = {40, 100.};
    } else if(filename.find("EJ2") != std::string::npos) {
       result = {80., 160};
    } else if(filename.find("EJ1") != std::string::npos) {
       result = {140., 200.}; 
    }
    return result;
}

void makePlotPearsonMg(std::string_view filename){
    auto ptrange = GetRangeForTrigger(filename);
    auto npanel = static_cast<int>((ptrange.fMax - ptrange.fMin)/20.),
         ncol = npanel / 3 + (npanel % 3 ? 1 : 0);
    std::cout << "Npanel : " << npanel << ",  ncol : " << ncol << std::endl; 
    auto plot = new TCanvas("plot", "plot", 1200, 400 * ncol);
    plot->Divide(3, ncol);

    auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data()));
    auto ptbins = ExtractPtBinning(*reader);

    int panel = 1;
    int bincounter(0);
    for(auto ptbin : ptbins){
        bincounter++;
        if(ptbin.fMax <= ptrange.fMin) continue;
        if(ptbin.fMin >= ptrange.fMax) break;
        plot->cd(panel);
        std::stringstream histname;
        histname << "pearsonmatrix_iter4_binpt" << (bincounter-1);

        auto pearsonmatrix  = static_cast<TH2 *>(reader->Get(histname.str().data()));
        pearsonmatrix->SetDirectory(nullptr);
        pearsonmatrix->GetXaxis()->SetTitle("m_{g}");
        pearsonmatrix->GetYaxis()->SetTitle("m_{g}");
        pearsonmatrix->SetTitle("");
        pearsonmatrix->SetStats(false);
        pearsonmatrix->Draw("colz");

        auto label = new TPaveText(0.35, 0.91, 0.89, 0.96, "NDC");
        label->SetBorderSize(0);
        label->SetFillStyle(0);
        label->SetTextFont(42);
        std::stringstream labeltext;
        labeltext.setf(ios::fixed, ios::floatfield);
        labeltext << std::setprecision(1) << ptbin.fMin << " GeV/c < p_{t,part} < " << ptbin.fMax << " GeV/c";
        label->AddText(labeltext.str().data());
        label->Draw();

        panel++;
    }

    plot->cd();
    plot->Update();
}