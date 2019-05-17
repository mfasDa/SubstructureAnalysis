#include "../../helpers/graphics.C"

TH1 *makeErrorRatioElianeR02R04(){
        std::cout << "Calling ratio 0.4" << std::endl;

   Double_t xAxis12[10] = {20, 30, 40, 50, 60, 70, 80, 100, 120, 140}; 
   TH1D *hJetShapeErrRatio1_Combined_copy__14 = new TH1D("ratio04","Unfold hResponseMatrixMain",9, xAxis12);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(1,0.3854477);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(2,0.4706764);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(3,0.5371839);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(4,0.5688113);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(5,0.5826412);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(6,0.6053351);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(7,0.6275919);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(8,0.6678327);
   hJetShapeErrRatio1_Combined_copy__14->SetBinContent(9,0.6952574);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(1,0.01321523);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(2,0.01496935);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(3,0.01762163);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(4,0.02112995);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(5,0.02598604);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(6,0.02657974);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(7,0.03259574);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(8,0.04861144);
   hJetShapeErrRatio1_Combined_copy__14->SetBinError(9,0.06397007);
   hJetShapeErrRatio1_Combined_copy__14->SetEntries(2252.41);
   hJetShapeErrRatio1_Combined_copy__14->SetDirectory(0);
   hJetShapeErrRatio1_Combined_copy__14->SetStats(0);
   return hJetShapeErrRatio1_Combined_copy__14;
}

TH1 *makeRatioElianeR02R04(){
   Double_t xAxis13[10] = {20, 30, 40, 50, 60, 70, 80, 100, 120, 140}; 
   
   TH1D *hDataRatio_NewBinning1_copy__15 = new TH1D("hDataRatio_NewBinning1_copy__15","Unfold hResponseMatrixMain",9, xAxis13);
   hDataRatio_NewBinning1_copy__15->SetBinContent(1,0.3854477);
   hDataRatio_NewBinning1_copy__15->SetBinContent(2,0.4706764);
   hDataRatio_NewBinning1_copy__15->SetBinContent(3,0.5371839);
   hDataRatio_NewBinning1_copy__15->SetBinContent(4,0.5688113);
   hDataRatio_NewBinning1_copy__15->SetBinContent(5,0.5826412);
   hDataRatio_NewBinning1_copy__15->SetBinContent(6,0.6053351);
   hDataRatio_NewBinning1_copy__15->SetBinContent(7,0.6275919);
   hDataRatio_NewBinning1_copy__15->SetBinContent(8,0.6678327);
   hDataRatio_NewBinning1_copy__15->SetBinContent(9,0.6952574);
   hDataRatio_NewBinning1_copy__15->SetBinError(1,0.003120069);
   hDataRatio_NewBinning1_copy__15->SetBinError(2,0.006338452);
   hDataRatio_NewBinning1_copy__15->SetBinError(3,0.0103348);
   hDataRatio_NewBinning1_copy__15->SetBinError(4,0.0152882);
   hDataRatio_NewBinning1_copy__15->SetBinError(5,0.02022488);
   hDataRatio_NewBinning1_copy__15->SetBinError(6,0.027006);
   hDataRatio_NewBinning1_copy__15->SetBinError(7,0.03757966);
   hDataRatio_NewBinning1_copy__15->SetBinError(8,0.0548993);
   hDataRatio_NewBinning1_copy__15->SetBinError(9,0.07601347);
   hDataRatio_NewBinning1_copy__15->SetEntries(2252.41);
   hDataRatio_NewBinning1_copy__15->SetDirectory(0);
   hDataRatio_NewBinning1_copy__15->SetStats(0);
   hDataRatio_NewBinning1_copy__15->SetDirectory(0);
   hDataRatio_NewBinning1_copy__15->SetStats(0);
    return hDataRatio_NewBinning1_copy__15 ;
}

void makePlotJetSpectraRatiosRootSPrelim(const std::string_view jetspectrafile = "jetspectrumratios.root"){
    std::unique_ptr<TFile> jetspectreader(TFile::Open(jetspectrafile.data(), "READ"));
    auto plot = new ROOT6tools::TSavableCanvas("crossSectionRatiosROOTSprelim", "Cross section ratios 13 TeV", 800, 700);
    plot->cd();
    gPad->SetTicks(1,1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);    
    gPad->SetTopMargin(0.05);
    auto frame = new ROOT6tools::TAxisFrame("ratioframe", "#it{p}_{T} (GeV/c)", "#frac{d#sigma^{#it{R}=0.2}}{d#it{p}_{T}d#eta} / #frac{d#sigma^{#it{R}=0.4}}{d#it{p}_{T}d#eta}", 0., 350., 0., 1.35);
    frame->Draw("axis");
    auto prelimlabel = new ROOT6tools::TNDCLabel(0.19, 0.73, 0.74, 0.94, "ALICE preliminary");
    prelimlabel->AddText("Jets, Anti-#it{k}_{T}");
    prelimlabel->AddText("#it{p}_{T}^{ch} > 0.15 GeV/c, #it{E}^{cluster} > 0.3 GeV");
    prelimlabel->AddText("|#eta^{tr}| < 0.7, |#eta^{cluster}| < 0.7, |#eta^{jet}| < 0.7 - #it{R}");
    prelimlabel->SetTextAlign(12);
    prelimlabel->Draw();
    auto rlegend = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.94, 0.4);
    rlegend->Draw();
    auto errlegend = new ROOT6tools::TDefaultLegend(0.15, 0.60, 0.57, 0.7);
    errlegend->Draw();

    auto line = new TLine(0., 1., 350., 1.);
    line->SetLineStyle(2);
    line->Draw();

    auto spec502 = makeRatioElianeR02R04();
    Style{kGreen+2, 21}.SetStyle<TH1>(*spec502);
    spec502->Draw("epsame");
    auto err502 = makeErrorRatioElianeR02R04();
    err502->SetFillColor(kGreen-8);
    err502->SetFillStyle(3001);
    err502->Draw("e2same");
    rlegend->AddEntry(spec502, "pp, #sqrt{s} = 5.02 TeV", "lep");

    std::string rstring = "R02R04";
    jetspectreader->cd(rstring.data());
    auto spec = static_cast<TH1 *>(gDirectory->Get(Form("stat_%s", rstring.data())));
    spec->SetDirectory(nullptr);
    auto corrUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("correlatedUncertainty_%s", rstring.data()))),
        shapeUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("shapeUncertainty_%s", rstring.data())));
    Style{kBlue, 20}.SetStyle<TH1>(*spec);
    spec->Draw("ex0psame");
    rlegend->AddEntry(spec, "pp, #sqrt{s} = 13 TeV", "lep");
    corrUncertainty->SetLineColor(kBlue);
    corrUncertainty->SetFillStyle(0);
    corrUncertainty->Draw("2same");
    errlegend->AddEntry(corrUncertainty, "Correlated uncertainties", "f");
    shapeUncertainty->SetFillColor(kBlue-9);
    shapeUncertainty->SetFillStyle(3001);
    shapeUncertainty->Draw("2same");
    errlegend->AddEntry(shapeUncertainty, "Shape uncertainties", "f");
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}