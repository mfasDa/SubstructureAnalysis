#ifndef __CLING__
#include <map>
#include <memory>
#include <string>
#include <RStringView.h>
#include <TFile.h>
#include <TH2.h>

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/msl.C"

TH2 *ReadUnfolded(const std::string_view inputfile){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    reader->cd("iteration4");
    auto hist = static_cast<TH2 *>(gDirectory->Get("zg_unfolded_iter4"));
    hist->SetDirectory(nullptr);
    return hist;
}
void checkImpactNEFcut(){
    std::string basefile = "JetSubstructureTree_FullJets_R02_INT7_unfolded_zg.root";
    std::map<std::string, TH2 *> unfolded = {{"no cut", ReadUnfolded(Form("no_cut/%s", basefile.data()))},
                                             {"low cut", ReadUnfolded(Form("lower_cut/%s", basefile.data()))},
                                             {"upper cut", ReadUnfolded(Form("upper_cut/%s", basefile.data()))},
                                             {"symmetric cut", ReadUnfolded(Form("symmetric_cut/%s", basefile.data()))}};

    auto plotabs  = new ROOT6tools::TSavableCanvas("plotabs", "Absolute counts in bin 0", 800., 600);
    plotabs->cd();
    (new ROOT6tools::TAxisFrame("absframe", "p_{t} (GeV/c)", "N_{bin0}", 0., 100., 0., 2000.))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.6, 0.65, 0.89, 0.89);
    leg->Draw();
    std::array<Color_t, 4> colors = {kRed, kBlue, kGreen, kBlack};
    std::array<Style_t, 4> markers = {24, 25, 26, 27};
    int icount = 0;
    for(auto h : unfolded){
        auto pro = h.second->ProjectionY(Form("ProAbs%s", h.first.data()), 1, 1);
        pro->SetDirectory(nullptr);
        Style{colors[icount], markers[icount]}.SetStyle<TH1>(*pro);
        icount++;
        pro->Draw("epsame");
        leg->AddEntry(pro, h.first.data(), "lep");
    }
    plotabs->Update();
    plotabs->SaveCanvas(plotabs->GetName());

    auto plotrel = new ROOT6tools::TSavableCanvas("plotrel", "Relative counts in bin 0", 800, 600);
    plotrel->cd();
    (new ROOT6tools::TAxisFrame("relframe", "p_{t} (GeV/c)", "N_{rel}", 0., 100., 0., 0.1))->Draw("axis");
    auto legrel = new ROOT6tools::TDefaultLegend(0.6, 0.65, 0.89, 0.89);
    legrel->Draw();
    icount = 0;
    for(auto h : unfolded){
        auto pro = h.second->ProjectionY(Form("ProRel%s", h.first.data()), 1, 1);
        pro->SetDirectory(nullptr);
        std::unique_ptr<TH1> weight(h.second->ProjectionY("Weight"));
        pro->Divide(weight.get());
        Style{colors[icount], markers[icount]}.SetStyle<TH1>(*pro);
        icount++;
        pro->Draw("epsame");
        leg->AddEntry(pro, h.first.data(), "lep");
    }
    plotrel->Update();
    plotrel->SaveCanvas(plotrel->GetName());
}