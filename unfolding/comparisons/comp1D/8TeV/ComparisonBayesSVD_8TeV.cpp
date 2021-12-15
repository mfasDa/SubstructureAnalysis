#include "../../../../meta/stl.C"
#include "../../../../meta/root.C"
#include "../../../../meta/root6tools.C"

#include "../../../../helpers/graphics.C"

#include "../../../../struct/JetSpectrumReader.cxx"
#include "../../../../struct/GraphicsPad.cxx"
#include "../../../../struct/Ratio.cxx"
#include "/home/austin/alice/QA_Jets_pp_8TeV/AnalysisSoftware/CommonHeaders/PlottingGammaConversionHistos.h"
#include "/home/austin/alice/QA_Jets_pp_8TeV/AnalysisSoftware/CommonHeaders/PlottingGammaConversionAdditional.h"

void ComparisonBayesSVD_8TeV(const std::string_view svdfile, const std::string_view bayesfile, string outputdir, string filetype = "png"){
    // Get spectra and radii
    const int regSVD = 6, regBayes = 4;
    std::vector<std::string> spectraBayes = {Form("normalized_reg%d", regBayes)}, spectraSVD =  {Form("normalized_reg%d", regSVD)};
    auto svddata = JetSpectrumReader(svdfile, spectraSVD),
         bayesdata = JetSpectrumReader(bayesfile, spectraBayes);
    auto jetradii = svddata.GetJetSpectra().GetJetRadii();
    int nrad = jetradii.size();

    // Define variables
    Style svdstyle{kBlack, 20}, bayesstyle{kRed, 25}, ratiostyle{kBlack, 20};

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

    for(auto r : jetradii) {
        // Define root objects
        canvas         = new TCanvas(Form("canvasR%02d", int(r * 10.)),"",0,0,1350,1500);
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

        SetStyleHistoTH2ForGraphs(dummyHistUpper, "#it{p}_{T} (GeV/#it{c})", "d#sigma/(dp_{T}dy) (mb/(GeV/c))", 0.85*textsizeLabelsWidth, textsizeLabelsWidth,
                                  0.85*textsizeLabelsWidth, textsizeLabelsWidth, 0.8,0.4/(textsizeFacWidth*margin), 512, 505,42,42);
        dummyHistUpper->DrawCopy();

        // Draw spectra
        GraphicsPad specpad(gPad);
        specpad.Legend(0.75, 0.80, 0.94, 0.95);
        specpad.Label(0.74, 0.65, 0.94, 0.75, "pp #sqrt{s} = 8 TeV");
        specpad.Label(0.82, 0.6, 0.94, 0.7, "Full Jets");
        specpad.Label(0.74, 0.55, 0.94, 0.65, Form("anti-k_{T}, R = %.1f", r));
        auto specSVD = (TH1D*)(svddata.GetJetSpectrum(r, spectraSVD[0])),
             specBayes = (TH1D*)(bayesdata.GetJetSpectrum(r, spectraBayes[0]));
        specSVD->SetMarkerSize(2);
        specBayes->SetMarkerSize(2);
        specpad.Draw<TH1>(specSVD, svdstyle, Form("SVD, reg=%d", regSVD), "p");
        specpad.Draw<TH1>(specBayes, bayesstyle, Form("Bayes, reg=%d", regBayes), "p");

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

        SetStyleHistoTH2ForGraphs(dummyHistLower, "#it{p}_{T} (GeV/#it{c})", "Bayes/SVD", 0.85*textsizeLabelsComp, textsizeLabelsComp, 0.85*textsizeLabelsComp,
                                  textsizeLabelsComp, 1.1, 0.4/(textsizeFacComp*margin),512,505,42,42);
        dummyHistLower->DrawCopy();

        // Draw ratio
        GraphicsPad ratiopad(gPad);
        auto methodratio = new Ratio(specBayes, specSVD);
        methodratio->SetMarkerSize(2);
        ratiopad.Draw<Ratio>(methodratio, ratiostyle);

        // Draw line at 1 on ratio
        DrawGammaLines(5.,350.,1.,1.,8.,16,9);

        // Update and save canvas
        canvas->Update();
        canvas->Print(Form("%s/comparisons/BayesSVD/comparisonBayesSvd_R%02d.%s", outputdir.c_str(), int(r * 10.), filetype.c_str()));

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
        SetStyleHistoTH2ForGraphs(dummyHistLower, "#it{p}_{T} (GeV/#it{c})", "Bayes/SVD", 0.5*textsizeLabelsComp, 0.63*textsizeLabelsComp, 0.5*textsizeLabelsComp,
                                  0.63*textsizeLabelsComp, 1.1, 0.7/(textsizeFacComp*margin),512,510,42,42);
        dummyHistLower->GetXaxis()->SetMoreLogLabels();
        dummyHistLower->DrawCopy();

        GraphicsPad ratiopad_ind(gPad);
        ratiopad_ind.Label(0.74, 0.87, 0.94, 0.97, "pp #sqrt{s} = 8 TeV");
        ratiopad_ind.Label(0.82, 0.82, 0.94, 0.92, "Full Jets");
        ratiopad_ind.Label(0.74, 0.77, 0.94, 0.87, Form("anti-k_{T}, R = %.1f", r));
        ratiopad_ind.Draw<Ratio>(methodratio, ratiostyle);

        DrawGammaLines(5.,350.,1.,1.,8.,16,9);

        ratioCanvas->Update();
        ratioCanvas->Print(Form("%s/comparisons/BayesSVD/RatioBayesSvd_R%02d.%s", outputdir.c_str(), int(r * 10.), filetype.c_str()));

        dummyHistLower = NULL;
        delete dummyHistLower;
    }
}
