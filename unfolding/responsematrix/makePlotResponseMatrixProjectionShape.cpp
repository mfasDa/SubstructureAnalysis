#ifndef __CLING__
#include <memory>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TH2.h>

#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/msl.C"

void makePlotResponseMatrixProjectionShape(const std::string_view inputfile, const std::string_view observablename){
    TH2 *response2D (nullptr), *hraw(nullptr), *htrue(nullptr);
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        hraw = static_cast<TH2 *>(reader->Get("hraw"));
        hraw->SetDirectory(nullptr);
        htrue = static_cast<TH2 *>(reader->Get("true"));
        htrue->SetDirectory(nullptr);
        response2D = static_cast<TH2 *>(reader->Get("ResponseMatrix2D"));
        response2D->SetDirectory(nullptr);
    }
    const int nbinspttrue = htrue->GetYaxis()->GetNbins(),
              nbinsptdet = hraw->GetYaxis()->GetNbins();

    auto tag = getFileTag(inputfile);
    auto jd = getJetType(tag);

    // 1. projection all pt
    auto plot_projectionpt = new ROOT6tools::TSavableCanvas(Form("responsematrix%s_allpt_%s", observablename.data(), tag.data()), "Repsonse matrix for shape all pt", 800, 600);
    plot_projectionpt->cd();
    auto projected = sliceResponseObservableBase(response2D, htrue, hraw, observablename.data());
    projected->SetDirectory(nullptr);
    projected->SetName(Form("responsematrix%s_allpt", observablename.data()));
    projected->SetStats(false);
    projected->SetXTitle(Form("%s_{measured}", observablename.data()));
    projected->SetYTitle(Form("%s_{true}", observablename.data()));
    projected->Draw("colz");
    (new ROOT6tools::TNDCLabel(0.05, 0.01, 0.25, 0.07, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
    plot_projectionpt->cd();
    plot_projectionpt->Update();
    plot_projectionpt->SaveCanvas(plot_projectionpt->GetName());

    // 2. slice ptpart
    auto plot_partslice = new ROOT6tools::TSavableCanvas(Form("responsematrix%s_sliceptpart_%s", observablename.data(), tag.data()), "Response matrix for shape sliced pt part", 1200, 1000);
    plot_partslice->DivideSquare(nbinspttrue);
    int ipad = 1; bool first = true;
    for(auto ipt : ROOT::TSeqI(0, nbinspttrue)){
        plot_partslice->cd(ipad++);
        auto slicetrue = sliceResponseObservableBase(response2D, htrue, hraw, observablename.data(), ipt);
        slicetrue->SetDirectory(nullptr);
        slicetrue->SetName(Form("responsematrix%s_slicepttrue%d", observablename.data(), ipt));
        slicetrue->SetStats(false);
        slicetrue->SetXTitle(Form("%s_{measured}", observablename.data()));
        slicetrue->SetYTitle(Form("%s_{true}", observablename.data()));
        slicetrue->Draw("colz");
        if(first) {
            (new ROOT6tools::TNDCLabel(0.05, 0.01, 0.35, 0.07, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
            first = false;
        }
    }
    plot_partslice->cd();
    plot_partslice->Update();
    plot_partslice->SaveCanvas(plot_partslice->GetName());

    auto plot_allslice = new ROOT6tools::TSavableCanvas(Form("responsematrixshape_sliceptall_%s", tag.data()), "Response matrix for shape sliced", 1200, 1000);
    plot_allslice->Divide(nbinsptdet, nbinspttrue);
    
    ipad = 1; first = true;
    for(auto iptdet : ROOT::TSeqI(0, nbinsptdet)){
        for(auto ipttrue : ROOT::TSeqI(0, nbinspttrue)){
            auto slicetrue = sliceResponseObservableBase(response2D, htrue, hraw, observablename.data(), ipttrue, iptdet);
            plot_allslice->cd(ipad++);
            slicetrue->SetDirectory(nullptr);
            slicetrue->SetName(Form("responsematrix%s_slicepttrue%d_sliceptdet%d", observablename.data(), ipttrue, iptdet));
            slicetrue->SetStats(false);
            slicetrue->SetXTitle(Form("%s_{measured}", observablename.data()));
            slicetrue->SetYTitle(Form("%s_{true}", observablename.data()));
            slicetrue->Draw("colz");
        }
    }
    plot_allslice->cd();
    plot_allslice->Update();
    plot_allslice->SaveCanvas(plot_allslice->GetName());
}