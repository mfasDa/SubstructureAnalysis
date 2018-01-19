#ifndef __CLING__
#include <array>
#include <memory>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TH2.h>
#include <TFile.h>
#endif

template<typename t>
t *Get(TFile &reader, const char *key){
    t *result(nullptr);
    result = static_cast<t *>(reader.Get(key));
    return result;
}

template<typename t>
std::unique_ptr<t> make_unique(t *object){
    std::unique_ptr<t> ptr(object);
    return ptr;
}

void plotJetPtCorrelation(std::string_view filename = "EnergyScale.root"){
    auto filereader = make_unique<TFile>(TFile::Open(filename.data(), "READ"));
    auto pt2d = Get<TH2>(*filereader, "ptdiff");

    TCanvas *plot = new TCanvas("ptcorrplot", "Reconstructed jet pt", 1000, 800);
    plot->Divide(3,2);
    std::array<double, 6> ptbins = {20, 40, 60, 80, 100, 160};

    const std::array<Color_t, 6> COLORS = {{kBlack, kRed, kGreen, kBlue, kOrange, kMagenta}};
    const std::array<Style_t, 6> MARKERS = {{24, 25, 26, 27, 28, 29}};

    int ipad = 1;
    for(auto trueptstep : ptbins){
        auto bin = pt2d->GetXaxis()->FindBin(trueptstep);
        auto ptmin = pt2d->GetXaxis()->GetBinLowEdge(bin), ptmax = pt2d->GetXaxis()->GetBinUpEdge(bin);
        auto ptdiff = pt2d->ProjectionY(Form("ptdiff%d", int(trueptstep)), bin, bin);
        ptdiff->SetDirectory(nullptr);
        ptdiff->SetTitle(Form("%.1f GeV/c < p_{t,jet,part} < %.1f GeV/c", ptmin, ptmax));
        ptdiff->Scale(1./ptdiff->Integral());
        ptdiff->SetStats(false);
        ptdiff->SetXTitle("(p_{t,det} - p_{t,part})/p_{t,part}");
        ptdiff->SetYTitle("1/N_{jet} dN/dJES");
        ptdiff->GetXaxis()->SetTitleOffset(1.5);
        ptdiff->SetMarkerColor(COLORS[ipad-1]);
        ptdiff->SetLineColor(COLORS[ipad-1]);
        ptdiff->SetMarkerStyle(MARKERS[ipad-1]);
        plot->cd(ipad++);
        gPad->SetGrid(0,0);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        ptdiff->Draw("pe");
    }
    plot->cd();
    plot->Update();
}
