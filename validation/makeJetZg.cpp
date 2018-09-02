#ifndef __CLING__
#include <array>
#include <map>
#include <RStringView.h>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/TSeq.hxx>

#include "TH1.h"

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TSavableCanvas.h"
#endif

#include "../helpers/msl.C"

TH1 *exrtactJetSpectrum(std::string_view filename){
    auto treename = GetNameJetSubstructureTree(filename);
    ROOT::RDataFrame daf(treename, filename);
    auto specrec = daf.Histo1D({"zgRec", "spectrum", 6, 0., 0.6}, "ZgMeasured", "PythiaWeight");

    auto result = histcopy(specrec.GetPtr());   
    result->SetDirectory(nullptr);
    return result;
}

void makeJetZg(){
    ROOT::EnableImplicitMT();
    std::map<double, TH1 *> spectra;
    for(auto r : ROOT::TSeqI(2, 6)) spectra[double(r)/10.] = exrtactJetSpectrum(Form("JetSubstructureTree_FullJets_R%02d_INT7_merged.root", r));

    auto plot = new ROOT6tools::TSavableCanvas("zgComparison", "zg comparison", 800, 600.);
    plot->cd();
    (new ROOT6tools::TAxisFrame("specaxis", "z_{g}", "d#sigma/dz_{g}", 0., 0.6, 0, 0.1))->Draw("axis");    
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
    leg->Draw();

    std::array<Color_t, 4> colors = {kRed, kBlue, kGreen, kBlack};
    std::array<Style_t, 4> markers = {24, 25, 26, 27};

    int icase = 0;
    for(auto s : spectra) {
        Style{colors[icase], markers[icase]}.SetStyle<TH1>(*s.second);
        s.second->Draw("epsame");
        leg->AddEntry(s.second, Form("Full Jets, R=%.1f", s.first), "lep");
        icase++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}