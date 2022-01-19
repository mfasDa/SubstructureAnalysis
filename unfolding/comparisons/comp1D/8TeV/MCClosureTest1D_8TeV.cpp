#include "../../../../meta/stl.C"
#include "../../../../meta/root.C"
#include "../../../../meta/root6tools.C"

#include "../../../../helpers/filesystem.C"
#include "../../../../helpers/graphics.C"
#include "../../../../helpers/math.C"
#include "../../../../helpers/root.C"
#include "../../../../helpers/string.C"
#include "../../../../struct/JetSpectrumReader.cxx"
#include "../../../../struct/GraphicsPad.cxx"
#include "../../../../struct/Ratio.cxx"
#include "/home/austin/alice/QA_Jets_pp_8TeV/AnalysisSoftware/CommonHeaders/PlottingGammaConversionHistos.h"
#include "/home/austin/alice/QA_Jets_pp_8TeV/AnalysisSoftware/CommonHeaders/PlottingGammaConversionAdditional.h"

void MCClosureTest1D_8TeV(const std::string_view inputfile, string outputdir, string filetype = "png", bool correctEffPure = true) {
    string closurename;
    if(correctEffPure) closurename = "partallclosure";
    else closurename = "partclosure";
    std::vector<std::string> spectra = {closurename}; 
    for(auto i : ROOT::TSeqI(1,11)) spectra.push_back(Form("unfoldedClosure_reg%d", i));
    auto data = JetSpectrumReader(inputfile, spectra);
    auto jetradii = data.GetJetSpectra().GetJetRadii();
    bool isSVD = (inputfile.find("SVD") != std::string::npos);

    std::array<Color_t, 10> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kMagenta, kGray, kAzure, kCyan};
    std::array<Style_t, 10> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};
    Style rawstyle{kBlack, 20};

    Double_t minPt               = 5.;
    Double_t maxPt               = 350.;
    Double_t textsizeLabelsWidth = 0;
    Double_t textsizeFacWidth    = 0;
    Double_t textsizeLabelsComp  = 0;
    Double_t textsizeFacComp     = 0;
    Double_t textSizeLabelsPixel = 50;

    Double_t arrayBoundariesX[2];
    Double_t arrayBoundariesY[4];
    Double_t relativeMarginsX[3];
    Double_t relativeMarginsY[4];

    ReturnCorrectValuesForCanvasScaling(1350,1500, 1, 3,0.11, 0.005, 0.005,0.085,arrayBoundariesX,arrayBoundariesY,relativeMarginsX,relativeMarginsY);
    Double_t margin = relativeMarginsX[0]*2.7*1350;

    // Declare root objects
    TCanvas *canvas;
    TCanvas *ratioCanvas;
    TPad    *upperPad;
    TPad    *lowerPad;
    TPad    *topPad;
    TH2F    *dummyHistUpper;
    TH2F    *dummyHistLower;

    gStyle->SetOptStat(0);

    for(auto r : jetradii){
        // Define root objects
        canvas         = new TCanvas(Form("canvasR%02d_%s", int(r * 10.), (isSVD ? "SVD" : "Bayes")),"",0,0,1350,1500);
        ratioCanvas    = new TCanvas(Form("ratioCanvasR%02d", int(r * 10.)),"",0,0,1650,1250);
        upperPad       = new TPad(Form("upperPadR%02d", int(r * 10.)), "", arrayBoundariesX[0], arrayBoundariesY[2], arrayBoundariesX[1], arrayBoundariesY[0],-1, -1, -2);
        lowerPad       = new TPad(Form("lowerPadR%02d", int(r * 10.)), "", arrayBoundariesX[0], arrayBoundariesY[3], arrayBoundariesX[1], arrayBoundariesY[2],-1, -1, -2);
        topPad         = new TPad(Form("topPadR%02d", int(r * 10.)), "", 0.13, 0.32, 0.52, 0.52,-1, -1, -2);
        dummyHistUpper = new TH2F(Form("dummyHistUpper_R%02d", int(r * 10.)),Form("dummyHistUpper_R%02d", int(r * 10.)), 1000, minPt,maxPt ,1000., .000000002,5);
        dummyHistLower = new TH2F(Form("dummyHistLower_R%02d", int(r * 10.)),Form("dummyHistLower_R%02d", int(r * 10.)), 1000, minPt,maxPt, 1000., 0.8,1.25);

        // Format root objects
        DrawGammaCanvasSettings( canvas,  0.13, 0.02, 0.03, 0.06);
        DrawGammaCanvasSettings( ratioCanvas,  0.12, 0.04, 0.03, 0.14);
        DrawGammaPadSettings( upperPad, relativeMarginsX[0], relativeMarginsX[2], relativeMarginsY[0], relativeMarginsY[1]);
        DrawGammaPadSettings( lowerPad, relativeMarginsX[0], relativeMarginsX[2], relativeMarginsY[1], relativeMarginsY[2]);
        DrawGammaPadSettings( topPad, 0., 0., 0., 0.);
        topPad->SetFillStyle(0);

        canvas->cd();
        upperPad->Draw();
        lowerPad->Draw();
        topPad->Draw();

        // Set up upper pad and dummy histo for spectra
        upperPad->cd();
        upperPad->SetLogy();
        upperPad->SetLogx();

        if (upperPad->XtoPixel(upperPad->GetX2()) < upperPad->YtoPixel(upperPad->GetY1())){
            textsizeLabelsWidth         = (Double_t)textSizeLabelsPixel/upperPad->XtoPixel(upperPad->GetX2()) ;
            textsizeFacWidth            = (Double_t)1./upperPad->XtoPixel(upperPad->GetX2()) ;
        } else {
            textsizeLabelsWidth         = (Double_t)textSizeLabelsPixel/upperPad->YtoPixel(upperPad->GetY1());
            textsizeFacWidth            = (Double_t)1./upperPad->YtoPixel(upperPad->GetY1());
        }

        SetStyleHistoTH2ForGraphs(dummyHistUpper, "#it{p}_{T} (GeV/#it{c})", "d#sigma/dp_{T} (mb/(GeV/c))", 0.85*textsizeLabelsWidth, textsizeLabelsWidth,
                                  0.85*textsizeLabelsWidth, textsizeLabelsWidth, 0.8,0.4/(textsizeFacWidth*margin), 512, 505,42,42);
        dummyHistUpper->DrawCopy();

        GraphicsPad specpad(gPad);
        specpad.Legend(0.8, 0.45, 0.99, 0.95);
        specpad.Label(0.6, 0.85, 0.8, 0.95, "pp #sqrt{s} = 8 TeV");
        specpad.Label(0.68, 0.8, 0.8, 0.9, "Full Jets");
        specpad.Label(0.6, 0.75, 0.8, 0.85, Form("anti-k_{T}, R = %.1f", r));
        auto *htruth = data.GetJetSpectrum(r, closurename);
        htruth->Scale(1., "width");
        htruth->SetMarkerSize(2);
        specpad.Draw<TH1>(htruth, rawstyle, "true", "p");

        // Set up lower pad and dummy histo for ratio
        lowerPad->cd();
        lowerPad->SetLogx();

      	if (lowerPad->XtoPixel(lowerPad->GetX2()) <lowerPad->YtoPixel(lowerPad->GetY1()) ){
      	    textsizeLabelsComp              = (Double_t)textSizeLabelsPixel/lowerPad->XtoPixel(lowerPad->GetX2()) ;
      	    textsizeFacComp                 = (Double_t)1./lowerPad->XtoPixel(lowerPad->GetX2()) ;
      	} else {
      	    textsizeLabelsComp              = (Double_t)textSizeLabelsPixel/lowerPad->YtoPixel(lowerPad->GetY1());
      	    textsizeFacComp                 = (Double_t)1./lowerPad->YtoPixel(lowerPad->GetY1());
      	}

        SetStyleHistoTH2ForGraphs(dummyHistLower, "#it{p}_{T} (GeV/#it{c})", "Unfolded/True", 0.85*textsizeLabelsComp, textsizeLabelsComp, 0.85*textsizeLabelsComp,
                                  textsizeLabelsComp, 1.1, 0.4/(textsizeFacComp*margin),512,505,42,42);
        dummyHistLower->DrawCopy();

        GraphicsPad ratiopad(gPad);
        for(auto ireg : ROOT::TSeqI(1, 11)){
            auto unfolded = data.GetJetSpectrum(r, Form("unfoldedClosure_reg%d", ireg));
            Style varstyle{colors[ireg-1], markers[ireg-1]};
            specpad.Draw<>(unfolded, varstyle, Form("reg=%d", ireg), "p");
            auto ratiotrue = new Ratio(unfolded, htruth);
            ratiopad.Draw<Ratio>(ratiotrue, varstyle);
        }

        // Draw line at 1 on ratio
        DrawGammaLines(5.,350.,1.,1.,8.,16,9);

        // Draw 5% uncertainty bars
        DrawGammaLines(5.,350.,1.05,1.05,6.,16,8);
        DrawGammaLines(5.,350.,0.95,0.95,6.,16,8);

        // Update and save canvas
        canvas->Update();
        canvas->Print(Form("%s/comparisons/Closure/MCClosureTest1D%s_R%02d.%s", outputdir.c_str(), (isSVD ? "Svd" : "Bayes"), int(r * 10.), filetype.c_str()));

        // Delete/clear root objects for next radius
        dummyHistUpper = NULL;
        delete dummyHistUpper;
        upperPad->Clear();
        lowerPad->Clear();
        topPad->Clear();
        canvas->Clear();

        // Make ratio-only plots
        ratioCanvas->cd();
        ratioCanvas->SetLogx();
        SetStyleHistoTH2ForGraphs(dummyHistLower, "#it{p}_{T} (GeV/#it{c})", "Unfolded/True", 0.5*textsizeLabelsComp, 0.63*textsizeLabelsComp, 0.5*textsizeLabelsComp,
                                  0.63*textsizeLabelsComp, 1.1, 0.7/(textsizeFacComp*margin),512,510,42,42);
        dummyHistLower->GetXaxis()->SetMoreLogLabels();
        dummyHistLower->DrawCopy();

        GraphicsPad ratiopad_ind(gPad);
        ratiopad_ind.Legend(0.15, 0.55, 0.35, 0.95);
        ratiopad_ind.Label(0.3, 0.87, 0.5, 0.97, "pp #sqrt{s} = 8 TeV");
        ratiopad_ind.Label(0.3, 0.82, 0.42, 0.92, "Full Jets");
        ratiopad_ind.Label(0.3, 0.77, 0.5, 0.87, Form("anti-k_{T}, R = %.1f", r));

        for(auto ireg : ROOT::TSeqI(1, 11)){
            auto unfolded = data.GetJetSpectrum(r, Form("unfoldedClosure_reg%d", ireg));
            Style varstyle{colors[ireg-1], markers[ireg-1]};
            auto ratiotrue = new Ratio(unfolded, htruth);
            ratiotrue->SetMarkerSize(2);
            ratiopad_ind.Draw<Ratio>(ratiotrue, varstyle, Form("reg=%d", ireg), "p");
        }

        DrawGammaLines(5.,350.,1.,1.,8.,16,9);

        // Draw 5% uncertainty bars
        DrawGammaLines(5.,350.,1.05,1.05,6.,16,8);
        DrawGammaLines(5.,350.,0.95,0.95,6.,16,8);

        ratioCanvas->Update();
        ratioCanvas->Print(Form("%s/comparisons/Closure/RatioClosure1D%s_R%02d.%s", outputdir.c_str(), (isSVD ? "Svd" : "Bayes"), int(r * 10.), filetype.c_str()));

        dummyHistLower = NULL;
        delete dummyHistLower;
    }
}
