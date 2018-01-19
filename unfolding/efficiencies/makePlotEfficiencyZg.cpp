#ifndef __CLING__
#include <array>
#include <memory>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TLegend.h>
#endif

void makePlotEfficiencyZg(std::string_view inputfile) {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data()));

    auto plot = new TCanvas("zgeff", "ZG efficiency", 800, 600);

    auto frame = new TH1F("effframe", "Efficiency frame", 300, 0., 300);
    frame->SetDirectory(nullptr);
    frame->SetStats(false);
    frame->GetXaxis()->SetTitle("p_{t} (GeV/c)");
    frame->GetYaxis()->SetTitle("#epsilon_{kine}");
    frame->GetYaxis()->SetRangeUser(0., 1.);
    frame->Draw("axis");

    auto leg = new TLegend(0.5, 0.15, 0.89, 0.5);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);
    leg->Draw();

    std::array<Color_t, 8> cols = {kRed, kBlue, kGreen, kOrange, kViolet, kGray, kTeal, kMagenta};
    std::array<Style_t, 8> markers = {24, 25, 26, 27, 28, 29, 30, 31};
    int istyle = 0;
    for(auto b : ROOT::TSeqI(10, 50, 5)){
        auto graph = static_cast<TGraph *>(reader->Get(Form("effzg_%d_%d", b, b+5)));
        graph->SetMarkerColor(cols[istyle]);
        graph->SetMarkerColor(cols[istyle]);
        graph->SetLineColor(cols[istyle]);
        graph->Draw("epsame");
        leg->AddEntry(graph, Form("%.2f < z_{g} < %.2f", float(b)/100., float(b+5)/100.), "lep");
        istyle++;
    }
    plot->Update();
}