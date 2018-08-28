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

#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

// Load standard library
#include "../../helpers/msl.C"

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

std::vector<PtBin> ExtractPtBinning(TFile &reader) {
    reader.cd("iteration4");
    auto hist = static_cast<TH2 *>(gDirectory->Get("zg_unfolded_iter4"));
    std::vector<PtBin> bins;
    for(auto b : ROOT::TSeqI(1, hist->GetYaxis()->GetNbins()+1)) bins.push_back({hist->GetYaxis()->GetBinLowEdge(b), hist->GetYaxis()->GetBinUpEdge(b)});
    std::sort(bins.begin(), bins.end(), std::less<PtBin>());
    return bins;
} 

void makePlotPearsonZgTemp(std::string_view filename){
    auto jd = getJetType(getFileTag(filename));
    auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data()));
    auto ptbins = ExtractPtBinning(*reader);
    auto npanel = ptbins.size(),
         ncol = npanel / 3 + (npanel % 3 ? 1 : 0);
    std::cout << "Npanel : " << npanel << ",  ncol : " << ncol << std::endl; 
    auto plot = new ROOT6tools::TSavableCanvas(Form("pearsonmatrixzg_%s_R%02d_%s", jd.fJetType.data(), int(jd.fJetRadius * 10.), jd.fTrigger.data()), "plot", 1200, 400 * ncol);
    plot->Divide(3, ncol);

    int panel = 1;
    int bincounter(0);
    for(auto ptbin : ptbins){
        bincounter++;
        plot->cd(panel);
        std::stringstream histname;
        histname << "pearsonmatrix_iter4_binzg" << (bincounter-1);

        auto pearsonmatrix  = static_cast<TH2 *>(gDirectory->Get(histname.str().data()));
        pearsonmatrix->SetDirectory(nullptr);
        pearsonmatrix->GetXaxis()->SetTitle("z_{g,part}");
        pearsonmatrix->GetYaxis()->SetTitle("z_{g,det}");
        pearsonmatrix->SetTitle("");
        pearsonmatrix->SetStats(false);
        pearsonmatrix->Draw("colz");

        std::stringstream labeltext;
        labeltext.setf(ios::fixed, ios::floatfield);
        labeltext << std::setprecision(1) << ptbin.fMin << " GeV/c < p_{t,part} < " << ptbin.fMax << " GeV/c";
        (new ROOT6tools::TNDCLabel(0.35, 0.91, 0.89, 0.96, labeltext.str().data()))->Draw();

        if(panel==1){
            (new ROOT6tools::TNDCLabel(0.05, 0.01, 0.4, 0.07, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
        }
        panel++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}