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

void makeComparisonIterationsZg(std::string_view inputfile){
    double ptmin = -1., ptmax = 1000.;
    int npanel = 12;
    if(inputfile.find("INT7") != std::string_view::npos) {
        ptmin = 40.;
        ptmax = 100.;
        npanel = (ptmax - ptmin) / 20.;
    } else if(inputfile.find("EJ2") != std::string::npos){
        ptmin = 80.;
        ptmax = 160.;
        npanel = (ptmax - ptmin) / 20.;
    } else if(inputfile.find("EJ1") != std::string_view::npos) {
        ptmin = 140.;
        ptmax = 200.;
        npanel = (ptmax - ptmin) / 20.;
    }
    auto ncol = npanel / 3 + (npanel % 3 ? 1 : 0);
    std::cout << "Npanel : " << npanel << ",  ncol : " << ncol << std::endl; 
    auto plot = new TCanvas("plot", "plot", 1200, 400 * ncol);
    plot->Divide(3, ncol);

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
        TString histname = TString::Format("zg_unfolded_iter%d.root", it);
        auto hinput = static_cast<TH2 *>(reader->Get(histname));
        if(!hinput) {
            std::cerr << "Input hist " << histname << " not found\n";
            reader->ls();
        }
        auto firstbin = hinput->GetYaxis()->FindBin(ptmin + 0.5), lastbin = hinput->GetYaxis()->FindBin(ptmax - 0.5);
        for(auto ip : ROOT::TSeqI(firstbin, lastbin+1)){
            plot->cd(ip - firstbin +1);
            gPad->SetLeftMargin(0.14);
            gPad->SetRightMargin(0.06);
            if(!isinit[ip-firstbin]){
                auto frame = new TH1F(Form("frame%d", ip), "; z_{g}; 1/N_{jet} dN/dz_{g}", 100, 0., 0.6);
                frame->SetDirectory(nullptr);
                frame->SetStats(false);
                frame->GetYaxis()->SetRangeUser(0., 0.4);
                frame->Draw("axis");

                auto label = new TPaveText(0.15, 0.77, 0.65, 0.89, "NDC");
                label->SetBorderSize(0);
                label->SetFillStyle(0);
                label->SetTextFont(42);
                label->AddText(Form("%.1f GeV/c < p_{t,j} < %.1f GeV/c", hinput->GetYaxis()->GetBinLowEdge(ip), hinput->GetYaxis()->GetBinUpEdge(ip)));
                label->Draw();

                if(ip-firstbin==0) leg->Draw();
                isinit[ip-firstbin] = kTRUE;
            }

            auto hslice = hinput->ProjectionX(Form("hSlice%d_niter%d", ip, it), ip, ip);
            hslice->SetDirectory(nullptr);
            hslice->SetStats(false);
            hslice->Scale(1./hslice->Integral());
            hslice->SetMarkerColor(COLORS.find(it)->second);
            hslice->SetLineColor(COLORS.find(it)->second);
            hslice->SetMarkerStyle(MARKERS.find(it)->second);
            hslice->Draw("epsame");
            if(ip-firstbin==0) leg->AddEntry(hslice, Form("niter=%d", it), "lep");
        }
    }

    // Draw Raw distribution on top
    auto hraw = static_cast<TH2 *>(reader->Get("hraw"));
    auto firstbin = hraw->GetYaxis()->FindBin(ptmin + 0.5), lastbin = hraw->GetYaxis()->FindBin(ptmax - 0.5);
    for(auto ip : ROOT::TSeqI(firstbin, lastbin+1)){
        plot->cd(ip-firstbin+1);
        auto hslice = hraw->ProjectionX(Form("hraw_%d", ip), ip, ip);
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