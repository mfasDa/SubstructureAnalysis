#ifndef __CLING__
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <RStringView.h>
#include <ROOT/TSeq.hxx>

#include <TCanvas.h>
#include <TFile.h>
#include <TH2.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TROOT.h>
#endif

void makeComparisonIterationsAngularity(std::string_view inputfile){
    TCanvas *plot = new TCanvas("plot", "plot", 1200, 800);
    plot->Divide(4,3);

    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data()));

    const std::array<int, 8> NITERPLOT = {{1, 5, 10, 15, 20, 15, 30, 34}};
    const std::map<int, Color_t> COLORS = {{1, kRed}, {5, kBlue}, {10, kGreen}, {15, kOrange}, {20, kViolet}, {25, kGray}, {30, kTeal}, {34, kMagenta}};
    const std::map<int, Marker_t> MARKERS = {{1, 24}, {5, 25}, {10, 26}, {15, 27}, {20, 28}, {25, 29}, {30, 30}, {34, 31}};
    gROOT->cd();
    std::array<bool, 12> isinit = {{kFALSE, kFALSE, kFALSE, kFALSE, kFALSE, kFALSE, kFALSE, kFALSE, kFALSE, kFALSE, kFALSE, kFALSE}};
    
    TLegend *leg = new TLegend(0.7, 0.3, 0.89, 0.89);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);

    for(auto it : NITERPLOT){
        TString histname = TString::Format("angularity_unfolded_iter%d.root", it);
        auto hinput = static_cast<TH2 *>(reader->Get(histname));
        if(!hinput) {
            std::cerr << "Input hist " << histname << " not found\n";
            reader->ls();
        }
        for(auto ip : ROOT::TSeqI(0,12)){
            plot->cd(ip+1);
            if(!isinit[ip]){
                auto frame = new TH1F(Form("frame%d", ip), "; g; 1/N_{jet} dN/dg", 100, 0., 0.2);
                frame->SetDirectory(nullptr);
                frame->SetStats(false);
                frame->GetYaxis()->SetRangeUser(0., 0.4);
                frame->Draw("axis");

                auto label = new TPaveText(0.15, 0.77, 0.65, 0.89, "NDC");
                label->SetBorderSize(0);
                label->SetFillStyle(0);
                label->SetTextFont(42);
                label->AddText(Form("%.1f GeV/c < p_{t,j} < %.1f GeV/c", hinput->GetYaxis()->GetBinLowEdge(ip+1), hinput->GetYaxis()->GetBinUpEdge(ip+1)));
                label->Draw();

                if(ip==0) leg->Draw();
                isinit[ip] = kTRUE;
            }

            auto hslice = hinput->ProjectionX(Form("hSlice%d_niter%d", ip, it), ip+1, ip+1);
            hslice->SetDirectory(nullptr);
            hslice->SetStats(false);
            hslice->Scale(1./hslice->Integral());
            hslice->SetMarkerColor(COLORS.find(it)->second);
            hslice->SetLineColor(COLORS.find(it)->second);
            hslice->SetMarkerStyle(MARKERS.find(it)->second);
            hslice->Draw("epsame");
            if(ip==0) leg->AddEntry(hslice, Form("niter=%d", it), "lep");
        }
    }

    // Draw Raw distribution on top
    auto hraw = static_cast<TH2 *>(reader->Get("hraw"));
    for(auto ip : ROOT::TSeqI(0, 12)){
        plot->cd(ip+1);
        auto hslice = hraw->ProjectionX(Form("hraw_%d", ip), ip+1, ip+1);
        hslice->SetDirectory(nullptr);
        hslice->SetStats(false);
        hslice->Scale(1./hslice->Integral());
        hslice->SetMarkerColor(kBlack);
        hslice->SetLineColor(kBlack);
        hslice->SetMarkerStyle(20);
        hslice->Draw("epsame");
        if(ip == 0) leg->AddEntry(hslice, "Raw");
        gPad->Update();
    }
    plot->cd();
    plot->Update();
}