#ifndef __CLING__
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH2.h>
#include <TLegend.h>
#include <TPaveText.h>
#endif

struct Range {
    double fMin;
    double fMax;
};

TH2 *Reduce(const TH2 * const data, const TH2 * const layout){
    auto reduced = static_cast<TH2 *>(layout->Clone(Form("%s_reduced", data->GetName())));
    reduced->Clear();
    for(auto xb : ROOT::TSeqI(1, reduced->GetXaxis()->GetNbins()+1)){
        for(auto yb : ROOT::TSeqI(1, reduced->GetYaxis()->GetNbins()+1)){
            reduced->SetBinContent(xb, yb, data->GetBinContent(data->GetXaxis()->FindBin(reduced->GetXaxis()->GetBinCenter(xb)), data->GetYaxis()->FindBin(reduced->GetYaxis()->GetBinCenter(yb))));
            reduced->SetBinError(xb, yb, data->GetBinError(data->GetXaxis()->FindBin(reduced->GetXaxis()->GetBinCenter(xb)), data->GetYaxis()->FindBin(reduced->GetYaxis()->GetBinCenter(yb))));
        }
    }
    return reduced;
}

std::map<int, TH2 *> extractRefoldingRatios(TFile &reader) {
    std::map<int, TH2 *> data;
    auto reference = static_cast<TH2 *>(reader.Get("hraw"));
    for(auto b : ROOT::TSeqI(1, 35)) {
        std::stringstream histname;
        histname << "zg_folded_iter" << b << ".root";
        std::cout << "histname: " << histname.str() << std::endl;
        auto unfolded = static_cast<TH2 *>(reader.Get(histname.str().data()));
        auto unfolded_reduced = Reduce(unfolded, reference);
        unfolded->GetYaxis()->SetRange(
            unfolded->GetYaxis()->FindBin(reference->GetYaxis()->GetBinLowEdge(1)+0.5),
            unfolded->GetYaxis()->FindBin(reference->GetYaxis()->GetBinUpEdge(reference->GetYaxis()->GetNbins()+1) -0.5)
        );
        auto ratio_unfolded_raw  = static_cast<TH2 *>(unfolded_reduced->Clone(Form("ratio_folded_raw_iter%d", b)));
        ratio_unfolded_raw->SetDirectory(nullptr);
        ratio_unfolded_raw->Divide(reference);
        data[b] = ratio_unfolded_raw;
    }
    return data;
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

void makeRefoldingTestZg(std::string_view filename){
    auto ptrange = GetRangeForTrigger(filename);
    auto npanel = static_cast<int>((ptrange.fMax - ptrange.fMin)/20.),
         ncol = npanel / 3 + (npanel % 3 ? 1 : 0);
    std::cout << "Npanel : " << npanel << ",  ncol : " << ncol << std::endl; 
    auto plot = new TCanvas("plot", "plot", 1200, 400 * ncol);
    plot->Divide(3, ncol);

    auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data()));
    auto ratiodata = extractRefoldingRatios(*reader);
    
    const std::array<int, 8> refiterations = {{1, 5, 10, 15, 20, 15, 30, 34}};
    std::map<int, Color_t> colors = {{1, kRed}, {5, kBlue}, {10, kGreen}, {15, kOrange}, {20, kViolet}, {25, kGray}, {30, kTeal}, {34, kMagenta}};
    std::map<int, Marker_t> markers = {{1, 24}, {5, 25}, {10, 26}, {15, 27}, {20, 28}, {25, 29}, {30, 30}, {34, 31}};
    int pad(1);
    TLegend *leg(nullptr);
    for(auto b : ROOT::TSeqI(ratiodata[1]->GetYaxis()->FindBin(ptrange.fMin+0.5), ratiodata[1]->GetYaxis()->FindBin(ptrange.fMax-0.5) + 1)){
        plot->cd(pad);
        if(pad == 1) {
            leg = new TLegend();
            leg->SetBorderSize(0);
            leg->SetFillStyle(0);
            leg->SetTextFont(42);
            leg->Draw();
        }
        auto axis = new TH1F(Form("ratioaxis%d",pad), "; z_{g}; folded/raw", 100, 0., 0.5);
        axis->SetDirectory(nullptr);
        axis->SetStats(false);
        axis->GetYaxis()->SetRangeUser(0.5, 1.5);
        axis->Draw("axis");
        auto ptlabel = new TPaveText(0.15, 0.15, 0.45, 0.22, "NDC");
        ptlabel->SetBorderSize(0);
        ptlabel->SetFillStyle(0);
        ptlabel->SetTextFont(42);
        ptlabel->AddText(Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ratiodata[1]->GetYaxis()->GetBinLowEdge(b), ratiodata[1]->GetYaxis()->GetBinUpEdge(b)));
        ptlabel->Draw();
        for(auto iter : refiterations){
            auto ratio2d = ratiodata[iter];
            auto hratio = ratio2d->ProjectionX(Form("%s_%d", ratio2d->GetName(),b), b, b);
            hratio->SetDirectory(nullptr);
            hratio->SetMarkerColor(colors[iter]);
            hratio->SetMarkerStyle(markers[iter]);
            hratio->SetLineColor(colors[iter]);
            hratio->Draw("epsame");
            if(pad == 1) {
                leg->AddEntry(hratio, Form("niter=%d", iter), "lep");
            }
            iter++;
        }
        pad++;
    }
    plot->cd();
    plot->Update();
}