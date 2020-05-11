#ifndef __CLING__
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <RStringView.h>
#include <ROOT/TSeq.hxx>
#include <TF1.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TGraphicsStyle.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/string.C"

struct Specname
{
  std::string tag;
  std::string detector;
  std::string trigger;
  std::string cluster;
};

struct fitresult {
  double val;
  double stat;
  double sys;
};

struct fitsettings {
  double min;
  double max;
  std::vector<double> varmin;
  std::vector<double> varmax;
};

std::map<TString, ROOT6tools::TGraphicsStyle> trgstyle = {
    {"MB", ROOT6tools::TGraphicsStyle(kBlack, 20)},
    {"MC7", ROOT6tools::TGraphicsStyle(kRed, 24)},
    {"G1", ROOT6tools::TGraphicsStyle(kOrange+2, 25)},
    {"G2", ROOT6tools::TGraphicsStyle(kViolet, 26)},
    {"J1", ROOT6tools::TGraphicsStyle(kBlue, 27)},
    {"J2", ROOT6tools::TGraphicsStyle(kGreen, 28)}};

Specname DecodeSpectrumName(const std::string_view specname)
{
  std::cout << "decoding " << specname << std::endl;
  auto tokens = tokenize(std::string(specname), '_');
  return {tokens[0], tokens[1], tokens[2], tokens[3]};
}

std::vector<TH1 *> ReadTriggers(const char *filename)
{
  std::vector<TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
  reader->cd("ANY");
  gDirectory->ls();
  for (auto spec : *(gDirectory->GetListOfKeys()))
  {
    auto specname = DecodeSpectrumName(spec->GetName());
    auto hist = static_cast<TH1 *>(static_cast<TKey *>(spec)->ReadObj());
    hist->SetDirectory(nullptr);

    if (specname.trigger == "MB")
    {
      std::stringstream histname;
      histname << (specname.detector == "EMCAL" ? "E" : "D") << specname.trigger;
      hist->SetName(histname.str().data());
    }
    else
    {
      hist->SetName(specname.trigger.data());
    }
    std::cout << "Adding histogram with name" << hist->GetName() << std::endl;
    result.emplace_back(hist);
  }
  return result;
}

fitresult DoFit(TH1 *data, fitsettings settings) {
  TF1 centmodel("model", "pol0", 0., 100.);
  data->Fit(&centmodel, "N", "", settings.min, settings.max);
  double val = centmodel.GetParameter(0);
  double error = centmodel.GetParError(0);
  std::vector<double> diffs;
  for(auto vmin : settings.varmin) {
    data->Fit(&centmodel, "N", "", vmin, settings.max);
    diffs.push_back(std::abs(centmodel.GetParameter(0) - val));
  }
  for(auto vmax : settings.varmax) {
    data->Fit(&centmodel, "N", "", settings.min, vmax);
    diffs.push_back(std::abs(centmodel.GetParameter(0) - val));
  }
  std::sort(diffs.begin(), diffs.end(), std::greater<double>());
  return {val, error, diffs[0]};
}

ROOT6tools::TSavableCanvas *EGAplot(const std::vector<TH1 *> &triggers){
    auto histfinder = [](const std::string_view histname) {
      return [histname](const TH1 *hist) { return std::string_view(hist->GetName()) == histname; };
    };
    auto mbrefEMCAL = *std::find_if(triggers.begin(), triggers.end(), histfinder("EMB")),
         mbrefDCAL = *std::find_if(triggers.begin(), triggers.end(), histfinder("DMB"));
    std::vector<std::string> emctriggers = {"MC7", "G2", "G1"};

    std::map<std::string, fitsettings> turnonsettings = {
      {"EG1", {12, 80, {10., 11., 13., 14., 15}, {60., 70., 90., 100.}}},
      {"EG2", {7, 80, {6., 8., 9., 10.}, {60., 70., 90., 100.}}},
      {"EMC7", {5, 50, {4., 6.}, {30., 40., 60.}}},
      {"DG1", {12, 80, {10., 11., 13., 14., 15}, {60., 70., 90., 100.}}},
      {"DG2", {7, 80, {6., 8., 9., 10.}, {60., 70., 90., 100.}}},
      {"DMC7", {5, 50, {4., 6.}, {30., 40., 60.}}}
    };

    auto plot = new ROOT6tools::TSavableCanvas("perfPlotEG1", "Performance plot EG1", 1200, 600);
    plot->Divide(2,1);

    // text settings 
    double textSizeUpperPanel = 0.055,
           textSizeLowerPanel = 0.1,
           titleOffsetYUpper = 1.3,
           titleOffsetYLower = 0.5,
           titleOffsetXLower = 1.2;

    double marginLeft = 0.13, marginRight = 0.04,
           marginTopUpper = 0.04, marginBottomUpper = 0.,
           marginTopLower = 0., marginBottomLower = 0.3;

    const char *xtitle = "E_{cl} (GeV)",
               *ytitleTurnon = "Trigger rejection",
               *ytitleRatio = "Ratio spectra";

    const char *emctitle = "EMCal", *dctitle = "DCal";

    double fitfillstyle = 3001;

    // EMCAL pad
    plot->cd(1);
    auto emcalpad = gPad;

    emcalpad->cd();
    auto turnonPadEmcal = new TPad("turnonPadEmcal", "EMCAL turnon pad", 0., 0.35, 1., 1.);
    turnonPadEmcal->Draw();
    turnonPadEmcal->cd();
    turnonPadEmcal->SetBottomMargin(marginBottomUpper);
    turnonPadEmcal->SetLeftMargin(marginLeft);
    turnonPadEmcal->SetRightMargin(marginRight);
    turnonPadEmcal->SetTopMargin(marginTopUpper);
    turnonPadEmcal->SetLogy();
    turnonPadEmcal->SetTicks(1,1);
    
    auto turnonAxisEmcal = new ROOT6tools::TAxisFrame("turnonAxisEmcal", xtitle, ytitleTurnon, 1e-3, 100., 0.8, 100000.);
    turnonAxisEmcal->GetYaxis()->SetTitleSize(textSizeUpperPanel);
    turnonAxisEmcal->GetYaxis()->SetLabelSize(textSizeUpperPanel);
    turnonAxisEmcal->GetYaxis()->SetTitleOffset(titleOffsetYUpper);
    turnonAxisEmcal->Draw("axis");

    auto aliperflabel = new ROOT6tools::TNDCLabel(0.5, 0.05, 0.94, 0.3, "ALICE Performance");
    aliperflabel->AddText("p-Pb, #sqrt{s_{NN}} = 8.16 TeV");
    aliperflabel->SetTextAlign(12);
    aliperflabel->SetTextSize(textSizeUpperPanel);
    aliperflabel->Draw();

    auto aliEmcalLabel = new ROOT6tools::TNDCLabel(0.77, 0.85, 0.94, 0.94, emctitle);
    aliEmcalLabel->SetTextSize(textSizeUpperPanel);
    aliEmcalLabel->Draw();

    auto aliEmcalLegend = new ROOT6tools::TDefaultLegend(0.17, 0.7, 0.7, 0.94);
    aliEmcalLegend->SetTextSize(textSizeUpperPanel);
    aliEmcalLegend->Draw();

    for(auto emctrg : emctriggers) {
      std::string triggername = "E" + emctrg;
      auto turnon = static_cast<TH1 *>((*std::find_if(triggers.begin(), triggers.end(), histfinder(triggername)))->Clone(Form("Turnon%s", triggername.data())));
      turnon->SetDirectory(nullptr);
      turnon->Divide(mbrefEMCAL);
      trgstyle[emctrg].DefineHistogram(turnon);
      turnon->Draw("epsame");
      auto res = DoFit(turnon, turnonsettings[triggername]);
      double combineduncertainty = std::sqrt(res.stat*res.stat + res.sys*res.sys);
      TBox *fitvalue = new TBox(0., res.val - combineduncertainty, 100., res.val + combineduncertainty);
      fitvalue->SetFillStyle(fitfillstyle);
      fitvalue->SetFillColor(trgstyle[emctrg].GetColor());
      fitvalue->Draw("same");
      aliEmcalLegend->AddEntry(turnon, Form("%s: %.1f #pm %.1f #pm %.1f", triggername.data(), res.val, res.stat, res.sys), "lep");
    }

    emcalpad->cd();
    auto ratioPadEmcal = new TPad("ratioPadEmcal", "EMCAL trigger ratio pad", 0., 0., 1., 0.35);
    ratioPadEmcal->Draw();
    ratioPadEmcal->cd();
    ratioPadEmcal->SetTopMargin(marginTopLower);
    ratioPadEmcal->SetLeftMargin(marginLeft);
    ratioPadEmcal->SetRightMargin(marginRight);
    ratioPadEmcal->SetBottomMargin(marginBottomLower);
    ratioPadEmcal->SetTicks(1,1);

    auto ratioAxisEmcal = new ROOT6tools::TAxisFrame("ratioAxisEmcal", xtitle, ytitleRatio,  1e-3, 100., 1e-3, 19.9);
    ratioAxisEmcal->GetXaxis()->SetTitleSize(textSizeLowerPanel);
    ratioAxisEmcal->GetXaxis()->SetLabelSize(textSizeLowerPanel);
    ratioAxisEmcal->GetYaxis()->SetTitleSize(textSizeLowerPanel);
    ratioAxisEmcal->GetYaxis()->SetLabelSize(textSizeLowerPanel);
    ratioAxisEmcal->GetXaxis()->SetTitleOffset(titleOffsetXLower);
    ratioAxisEmcal->GetYaxis()->SetTitleOffset(titleOffsetYLower);
    ratioAxisEmcal->Draw("axis");

    auto ratioEmcalLegend = new ROOT6tools::TDefaultLegend(0.15, 0.7, 0.7, 0.98);
    ratioEmcalLegend->SetTextSize(textSizeLowerPanel);
    ratioEmcalLegend->Draw();

    auto ratioEG1EG2 = static_cast<TH1 *>((*std::find_if(triggers.begin(), triggers.end(), histfinder("EG1")))->Clone("RatioEG1EG2"));
    ratioEG1EG2->SetDirectory(nullptr);
    ratioEG1EG2->Divide(*std::find_if(triggers.begin(), triggers.end(), histfinder("EG2")));
    trgstyle["G1"].DefineHistogram(ratioEG1EG2);
    ratioEG1EG2->Draw("epsame");
    auto resEG1EG2 = DoFit(ratioEG1EG2, turnonsettings["EG1"]);
    double combEG1EG2 = std::sqrt(resEG1EG2.stat*resEG1EG2.stat + resEG1EG2.sys*resEG1EG2.sys);
    TBox *fEG1EG2 = new TBox(0., resEG1EG2.val - combEG1EG2, 100., resEG1EG2.val + combEG1EG2);
    fEG1EG2->SetFillStyle(fitfillstyle);
    fEG1EG2->SetFillColor(trgstyle["G1"].GetColor());
    fEG1EG2->Draw("same");
    ratioEmcalLegend->AddEntry(ratioEG1EG2, Form("EG1 / EG2: %.1f #pm %.1f #pm %.1f", resEG1EG2.val, resEG1EG2.stat, resEG1EG2.sys), "lep");

    auto ratioEG2EMC7 = static_cast<TH1 *>((*std::find_if(triggers.begin(), triggers.end(), histfinder("EG2")))->Clone("RatioEG2EMC7"));
    ratioEG2EMC7->SetDirectory(nullptr);
    ratioEG2EMC7->Divide(*std::find_if(triggers.begin(), triggers.end(), histfinder("EMC7")));
    trgstyle["G2"].DefineHistogram(ratioEG2EMC7);
    ratioEG2EMC7->Draw("epsame");
    auto resEG2EMC7 = DoFit(ratioEG2EMC7, turnonsettings["EG2"]);
    double combEG2EMC7 = std::sqrt(resEG2EMC7.stat*resEG2EMC7.stat + resEG2EMC7.sys*resEG2EMC7.sys);
    TBox *fEG2EMC7 = new TBox(0., resEG2EMC7.val - combEG2EMC7, 100., resEG2EMC7.val + combEG2EMC7);
    fEG2EMC7->SetFillStyle(fitfillstyle);
    fEG2EMC7->SetFillColor(trgstyle["G2"].GetColor());
    fEG2EMC7->Draw("same");
    ratioEmcalLegend->AddEntry(ratioEG2EMC7, Form("EG2 / EMC7: %.1f #pm %.1f #pm %.1f", resEG2EMC7.val, resEG2EMC7.stat, resEG2EMC7.sys), "lep");

    // DCAL pad
    plot->cd(2);
    auto dcalpad = gPad;

    dcalpad->cd();
    auto turnonPadDcal = new TPad("turnonPadDcal", "DCAL turnon pad", 0., 0.35, 1., 1.);
    turnonPadDcal->Draw();
    turnonPadDcal->cd();
    turnonPadDcal->SetBottomMargin(marginBottomUpper);
    turnonPadDcal->SetLeftMargin(marginLeft);
    turnonPadDcal->SetRightMargin(marginRight);
    turnonPadDcal->SetTopMargin(marginTopUpper);
    turnonPadDcal->SetLogy();
    turnonPadDcal->SetTicks(1,1);
    
    auto turnonAxisDcal = new ROOT6tools::TAxisFrame("turnonAxisDcal", xtitle, ytitleTurnon, 1e-3, 100., 0.8, 100000.);
    turnonAxisDcal->GetYaxis()->SetTitleSize(textSizeUpperPanel);
    turnonAxisDcal->GetYaxis()->SetLabelSize(textSizeUpperPanel);
    turnonAxisDcal->GetYaxis()->SetTitleOffset(titleOffsetYUpper);
    turnonAxisDcal->Draw("axis");

    auto aliDcalLabel = new ROOT6tools::TNDCLabel(0.77, 0.85, 0.94, 0.94, dctitle);
    aliDcalLabel->SetTextSize(textSizeUpperPanel);
    aliDcalLabel->Draw();

    auto aliDcalLegend = new ROOT6tools::TDefaultLegend(0.17, 0.7, 0.7, 0.94);
    aliDcalLegend->SetTextSize(textSizeUpperPanel);
    aliDcalLegend->Draw();

    for(auto emctrg : emctriggers) {
      std::string triggername = "D" + emctrg;
      auto turnon = static_cast<TH1 *>((*std::find_if(triggers.begin(), triggers.end(), histfinder(triggername)))->Clone(Form("Turnon%s", triggername.data())));
      turnon->SetDirectory(nullptr);
      turnon->Divide(mbrefDCAL);
      trgstyle[emctrg].DefineHistogram(turnon);
      turnon->Draw("epsame");
      auto res = DoFit(turnon, turnonsettings[triggername]);
      double combineduncertainty = std::sqrt(res.stat*res.stat + res.sys*res.sys);
      TBox *fitvalue = new TBox(0., res.val - combineduncertainty, 100., res.val + combineduncertainty);
      fitvalue->SetFillStyle(fitfillstyle);
      fitvalue->SetFillColor(trgstyle[emctrg].GetColor());
      fitvalue->Draw("same");
      aliDcalLegend->AddEntry(turnon, Form("%s: %.1f #pm %.1f #pm %.1f", triggername.data(), res.val, res.stat, res.sys), "lep");
    }

    dcalpad->cd();
    auto ratioPadDcal = new TPad("ratioPadDcal", "DCAL trigger ratio pad", 0., 0., 1., 0.35);
    ratioPadDcal->Draw();
    ratioPadDcal->cd();
    ratioPadDcal->SetTopMargin(marginTopLower);
    ratioPadDcal->SetLeftMargin(marginLeft);
    ratioPadDcal->SetRightMargin(marginRight);
    ratioPadDcal->SetBottomMargin(marginBottomLower);
    ratioPadDcal->SetTicks(1,1);

    auto ratioAxisDcal = new ROOT6tools::TAxisFrame("ratioAxisDcal", xtitle, ytitleRatio,  1e-3, 100., 1e-3, 19.9);
    ratioAxisDcal->GetXaxis()->SetTitleSize(textSizeLowerPanel);
    ratioAxisDcal->GetXaxis()->SetLabelSize(textSizeLowerPanel);
    ratioAxisDcal->GetYaxis()->SetTitleSize(textSizeLowerPanel);
    ratioAxisDcal->GetYaxis()->SetLabelSize(textSizeLowerPanel);
    ratioAxisDcal->GetXaxis()->SetTitleOffset(titleOffsetXLower);
    ratioAxisDcal->GetYaxis()->SetTitleOffset(titleOffsetYLower);
    ratioAxisDcal->Draw("axis");

    auto ratioDcalLegend = new ROOT6tools::TDefaultLegend(0.15, 0.7, 0.7, 0.98);
    ratioDcalLegend->SetTextSize(textSizeLowerPanel);
    ratioDcalLegend->Draw();

    auto ratioDG1DG2 = static_cast<TH1 *>((*std::find_if(triggers.begin(), triggers.end(), histfinder("DG1")))->Clone("RatioDG1DG2"));
    ratioDG1DG2->SetDirectory(nullptr);
    ratioDG1DG2->Divide(*std::find_if(triggers.begin(), triggers.end(), histfinder("DG2")));
    trgstyle["G1"].DefineHistogram(ratioDG1DG2);
    ratioDG1DG2->Draw("epsame");
    auto resDG1DG2 = DoFit(ratioDG1DG2, turnonsettings["DG1"]);
    double combDG1DG2 = std::sqrt(resDG1DG2.stat*resDG1DG2.stat + resDG1DG2.sys*resDG1DG2.sys);
    TBox *fDG1DG2 = new TBox(0., resDG1DG2.val - combDG1DG2, 100., resDG1DG2.val + combDG1DG2);
    fDG1DG2->SetFillStyle(fitfillstyle);
    fDG1DG2->SetFillColor(trgstyle["G1"].GetColor());
    fDG1DG2->Draw("same");
    ratioDcalLegend->AddEntry(ratioDG1DG2, Form("DG1 / DG2: %.1f #pm %.1f #pm %.1f", resDG1DG2.val, resDG1DG2.stat, resDG1DG2.sys), "lep");

    auto ratioDG2DMC7 = static_cast<TH1 *>((*std::find_if(triggers.begin(), triggers.end(), histfinder("DG2")))->Clone("RatioDG2DMC7"));
    ratioDG2DMC7->SetDirectory(nullptr);
    ratioDG2DMC7->Divide(*std::find_if(triggers.begin(), triggers.end(), histfinder("DMC7")));
    trgstyle["G2"].DefineHistogram(ratioDG2DMC7);
    ratioDG2DMC7->Draw("epsame");
    auto resDG2DMC7 = DoFit(ratioDG2DMC7, turnonsettings["EG2"]);
    double combDG2DMC7 = std::sqrt(resDG2DMC7.stat*resDG2DMC7.stat + resDG2DMC7.sys*resDG2DMC7.sys);
    TBox *fDG2DMC7 = new TBox(0., resDG2DMC7.val - combDG2DMC7, 100., resDG2DMC7.val + combDG2DMC7);
    fDG2DMC7->SetFillStyle(fitfillstyle);
    fDG2DMC7->SetFillColor(trgstyle["G2"].GetColor());
    fDG2DMC7->Draw("same");
    ratioDcalLegend->AddEntry(ratioDG2DMC7, Form("DG2 / DMC7: %.1f #pm %.1f #pm %.1f", resDG2DMC7.val, resDG2DMC7.stat, resDG2DMC7.sys), "lep");

    plot->cd();

    return plot;
}

void  ClusterTurnonpPb_PerfPaper(const char *filename = "ClusterSpectra.root"){
  auto data = ReadTriggers(filename);
  auto egaplot = EGAplot(data);
  egaplot->SaveCanvas("AlicePerf_Trg_EGA_pPb816TeV");
}