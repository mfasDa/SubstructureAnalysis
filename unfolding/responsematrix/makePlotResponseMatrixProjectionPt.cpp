#ifndef __CLING__
#include <memory>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TH2.h>

#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/msl.C"

void makePlotResponseMatrixProjectionPt(const std::string_view inputfile, const std::string_view observablename){
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
    const int nbinsshapetrue = htrue->GetXaxis()->GetNbins(),
              nbinsshapedet = hraw->GetXaxis()->GetNbins();

    auto tag = getFileTag(inputfile);
    auto jd = getJetType(tag);

    // 1. projection all pt
    auto plot_projectionpt = new ROOT6tools::TSavableCanvas(Form("responsematrixpt_all%s_%s", observablename.data(), tag.data()), "Repsonse matrix for pt all shape", 800, 600);
    plot_projectionpt->cd();
    gPad->SetLogz();
    auto projected = sliceResponsePtBase(response2D, htrue, hraw, observablename.data());
    projected->SetDirectory(nullptr);
    projected->SetName(Form("responsematrixpt_all%s", observablename.data()));
    projected->SetStats(false);
    projected->SetXTitle("p_{t,measured} (GeV/c)");
    projected->SetYTitle("p_{t,true} (GeV/c)");
    projected->Draw("colz");
    (new ROOT6tools::TNDCLabel(0.05, 0.01, 0.25, 0.07, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
    plot_projectionpt->cd();
    plot_projectionpt->Update();
    plot_projectionpt->SaveCanvas(plot_projectionpt->GetName());

    // 2. slice ptpart
    auto plot_partslice = new ROOT6tools::TSavableCanvas(Form("responsematrixpt_slice%spart_%s", observablename.data(), tag.data()), "Response matrix for pt sliced shape part", 1200, 1000);
    plot_partslice->DivideSquare(nbinsshapetrue);
    int ipad = 1; bool first = true;
    for(auto ishape : ROOT::TSeqI(0, nbinsshapetrue)){
        plot_partslice->cd(ipad++);
        gPad->SetLogz();
        auto slicetrue = sliceResponsePtBase(response2D, htrue, hraw, observablename.data(), ishape);
        slicetrue->SetDirectory(nullptr);
        slicetrue->SetName(Form("responsematrixpt_slice%strue%d", observablename.data(), ishape));
        slicetrue->SetStats(false);
        slicetrue->SetXTitle("p_{t,measured} (GeV/c)");
        slicetrue->SetYTitle("p_{t,true} (GeV/c)");
        slicetrue->Draw("colz");
        if(first) {
            (new ROOT6tools::TNDCLabel(0.05, 0.01, 0.35, 0.07, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
            first = false;
        }
    }
    plot_partslice->cd();
    plot_partslice->Update();
    plot_partslice->SaveCanvas(plot_partslice->GetName());

    auto plot_allslice = new ROOT6tools::TSavableCanvas(Form("responsematrixpt_slice%sall_%s", observablename.data(), tag.data()), "Response matrix for pt sliced shape all", 1200, 1000);
    plot_allslice->Divide(nbinsshapedet, nbinsshapetrue);
    
    ipad = 1; first = true;
    for(auto ishapedet : ROOT::TSeqI(0, nbinsshapedet)){
        for(auto ishapetrue : ROOT::TSeqI(0, nbinsshapetrue)){
            auto slicetrue = sliceResponsePtBase(response2D, htrue, hraw, observablename.data(), ishapetrue, ishapedet);
            plot_allslice->cd(ipad++);
            gPad->SetLogz();
            slicetrue->SetDirectory(nullptr);
            slicetrue->SetName(Form("responsematrixpt_slice%strue%d_slice%sdet%d", observablename.data(), ishapetrue, observablename.data(), ishapedet));
            slicetrue->SetStats(false);
            slicetrue->SetXTitle("p_{t,measured} (GeV/c)");
            slicetrue->SetYTitle("p_{t,true} (GeV/c)");
            slicetrue->Draw("colz");
        }
    }
    plot_allslice->cd();
    plot_allslice->Update();
    plot_allslice->SaveCanvas(plot_allslice->GetName());
}