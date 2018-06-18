#ifndef __CLING__
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "RStringView.h"
#include <TF1.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TList.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/math.C"

std::vector<TH1 *> getNormalizedSpectra(const std::string_view filename, const std::string_view dirname, const std::string_view triggercluster){
  const std::vector<std::string> kEMCALtriggers = {"EG1", "EG2", "EJ1", "EJ2"};
  std::vector<TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  reader->cd(dirname.data());
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  std::vector<TObject *> histvec;     // for std::find_if
  for(auto o : *histlist) histvec.emplace_back(o);

  // get min bias spectra
  auto eventcounterMB = static_cast<TH1 *>(histlist->FindObject(Form("hEventCounterMB")));
  auto mbga = static_cast<TH1 *>(histlist->FindObject("hPatchADCEGAMB")),
       mbje = static_cast<TH1 *>(histlist->FindObject("hPatchADCEJEMB"));
  mbga->SetDirectory(nullptr);
  mbga->SetName("MB_EGA");
  mbga->Scale(1./eventcounterMB->GetBinContent(1));
  normalizeBinWidth(mbga);
  result.emplace_back(mbga);
  mbje->SetDirectory(nullptr);
  mbje->SetName("MB_EJE");
  mbje->Scale(1./eventcounterMB->GetBinContent(1));
  normalizeBinWidth(mbje);
  result.emplace_back(mbje);

  for(const auto &t : kEMCALtriggers){
    bool isJE = t.find("J") != std::string::npos;
    auto normtrg = static_cast<TH1 *>(histlist->FindObject(Form("hEventCounter%s%s", t.data(), triggercluster.data())));
    auto spectrg = static_cast<TH1 *>(*std::find_if(histvec.begin(), histvec.end(), [&] (const TObject *hist) {
      const std::string_view histname = hist->GetName();
      return histname.find("hPatchADC") != std::string::npos && histname.find(t) != std::string::npos && histname.find(triggercluster) != std::string::npos;
    }));
    spectrg->SetDirectory(nullptr);
    spectrg->SetName(Form("%s_%s", t.data(), isJE ? "EJE" : "EGA"));
    spectrg->Scale(1./normtrg->GetBinContent(1));
    normalizeBinWidth(spectrg);
    result.emplace_back(spectrg);
  }
  return result;
}

ROOT6tools::TSavableCanvas *MakeRatioPlot(const std::vector<TH1 *> &data){
  std::array<std::vector<std::string>, 2> triggers = {{{"EG2_EGA", "EG1_EGA"}, {"EJ2_EJE", "EJ1_EJE"}}};
  auto plot = new ROOT6tools::TSavableCanvas("RecalcPatchSpectraRatios", "spectraRatios", 1200, 600);
  plot->Divide(2,1);

  std::array<Style, 2> styles = {{{kRed, 24}, {kBlue, 25}}};
  std::map<std::string, std::pair<double, double>> fitranges = {{"EGA", {120, 500}}, {"EJE", {300., 700.}}};
  int ipad = 1;
  for(const auto &t : triggers){
    const std::string_view patchtype = t[0].substr(t[0].find("_")+1);
    plot->cd(ipad++);
    (new ROOT6tools::TAxisFrame(Form("spectraRatioFrame%s", patchtype.data()), "ADC", "Spectra ratio", 0., 1000., 0., 30.))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.65, 0.75, 0.89, 0.82, Form("%s patches", patchtype.data())))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.7, 0.55, 0.89);
    leg->Draw();
    for(auto itrg : ROOT::TSeqI(0, t.size()-1)){
      const auto &numhistname = t[itrg+1], &denhistname = t[itrg];
      auto trgnamenum = numhistname.substr(0, numhistname.find('_')-1), trgnameden = t[itrg].substr(0, t[itrg].find('_')-1);
      auto specnum = *std::find_if(data.begin(), data.end(), [&numhistname](const TH1 *hist) { return std::string_view(hist->GetName()) == numhistname; });
      auto specden = *std::find_if(data.begin(), data.end(), [&denhistname](const TH1 *hist) { return std::string_view(hist->GetName()) == denhistname; });
      auto ratio = static_cast<TH1 *>(specnum->Clone(Form("Ratio_%s_%s", trgnamenum.data(), trgnameden.data())));
      ratio->SetDirectory(nullptr);
      ratio->Divide(specden);
      styles[itrg].SetStyle<TH1>(*ratio);
      ratio->Draw("epsame");

      // Fit ratio
      auto fit = new TF1(Form("FitRatio_%s_%s", trgnamenum.data(), trgnameden.data()), "pol0", 100., 1000.);
      auto fitrange = fitranges[std::string(patchtype)];
      ratio->Fit(fit, "N", "", fitrange.first, fitrange.second);
      fit->SetLineColor(styles[itrg].color);
      fit->SetLineStyle(2);
      fit->Draw("lsame");
      leg->AddEntry(ratio, Form("%s/%s: %.2f #pm %.3f", trgnamenum.data(), trgnameden.data(), fit->GetParameter(0), fit->GetParError(0)), "lep");
    }
  }
  plot->cd();
  plot->Update();
  return plot;
}

ROOT6tools::TSavableCanvas *MakeTurnonPlot(const std::vector<TH1 *> &data){
  auto plot = new ROOT6tools::TSavableCanvas("RecalcPatchTurnon", "Turnon curve from recalc patches", 1200, 600);
  plot->Divide(2,1);
  
  const std::array<std::string, 2> kPatchTypes = {{"EGA", "EJE"}};
  int ipad = 1;
  
  std::array<Style, 2> thresholds = {{{kRed, 24}, {kBlue, 25}}};
  std::map<std::string, std::pair<double, double>> fitranges = {{"EGA", {120, 500}}, {"EJE", {300., 700.}}};
  for(const auto &pt : kPatchTypes) {
    plot->cd(ipad++);
    (new ROOT6tools::TAxisFrame(Form("TurnonFrame_%s", pt.data()), "ADC", "Trigger rejection", 0, 1000., 0., 10000.))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.82, 0.35, 0.89, Form("%s patches", pt.data())))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.55, 0.75, 0.89, 0.89);
    leg->Draw();

    auto mbref = *std::find_if(data.begin(), data.end(), [&pt](const TH1 *hist){
      std::string_view histname = hist->GetName();
      return histname.find("MB") != std::string::npos && histname.find(pt) != std::string::npos;
    });
    for(auto thresh : ROOT::TSeqI(0,2)){
      auto trghist = *std::find_if(data.begin(), data.end(), [thresh, &pt](const TH1 *hist) {
        std::string_view histname = hist->GetName();
        return histname.find(Form("%d", thresh+1)) != std::string::npos && histname.find(pt) != std::string::npos;
      });
      auto histname = std::string_view(trghist->GetName());
      auto trgname = histname.substr(0, histname.find("_"));
      auto turnon = static_cast<TH1 *>(trghist->Clone(Form("Turnon%s", trgname.data())));
      turnon->Divide(mbref);
      thresholds[thresh].SetStyle<TH1>(*turnon);
      turnon->Draw("epsame");

      auto fit = new TF1(Form("turnonfit_%s", trgname.data()), "pol0", 100., 1000.);
      auto fitrange = fitranges[pt];
      turnon->Fit(fit, "N", "", fitrange.first, fitrange.second);
      fit->SetLineColor(thresholds[thresh].color);
      fit->SetLineStyle(2);
      fit->Draw("lsame");
      leg->AddEntry(turnon, Form("%s: %.1f #pm %.1f", trgname.data(), fit->GetParameter(0), fit->GetParError(0)), "lep");
    }
  }

  plot->cd();
  plot->Update();
  return plot;
}

ROOT6tools::TSavableCanvas *MakeSpectraPlot(const std::vector<TH1 *> &data){
  std::array<std::vector<std::string>, 2> triggers = {{{"MB_EGA", "EG2_EGA", "EG1_EGA"}, {"MB_EJE", "EJ2_EJE", "EJ1_EJE"}}};
  std::unordered_map<std::string, Style> kStyles  = {{"MB", {kBlack, 20}}, {"EG1", {kRed, 24}}, {"EG2", {kOrange, 25}}, {"EJ1", {kBlue, 26}}, {"EJ2", {kGreen, 27}}};
  auto plot = new ROOT6tools::TSavableCanvas("RecalcPatchSpectra", "Spectra of recalc patches", 1200, 600);
  plot->Divide(2,1);

  int ipad = 1;
  for(const auto &t : triggers){
    plot->cd(ipad++);
    gPad->SetLogy();
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.06);
    const auto &patchtype = t[0].substr(t[0].find('_')+1);
    (new ROOT6tools::TAxisFrame(Form("SpectrumFrame_%s", patchtype.data()), "ADC", "1/N_{trg} dN/dADC", 0., 1000., 1e-7, 100))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("%s patches", patchtype.data())))->Draw("epsame");
    auto leg = new ROOT6tools::TDefaultLegend(0.75, 0.7, 0.89, 0.89);
    leg->Draw();
    for(auto s : t){
      auto triggername = s.substr(0, s.find('_'));
      auto spec = *std::find_if(data.begin(), data.end(), [&s](const TH1 *hist) { return std::string_view(hist->GetName()) == s; });
      auto histstyle = kStyles.find(triggername);
      histstyle->second.SetStyle<TH1>(*spec);
      spec->Draw("epsame");
      leg->AddEntry(spec, triggername.data(), "lep");
    }
  }
  plot->cd();
  plot->Update();
  return plot;
}

void makeRejection(const std::string_view dirname, const std::string_view filename, const std::string_view triggercluster = "ANY"){
  auto data = getNormalizedSpectra(filename, dirname, triggercluster);
  auto specplot = MakeSpectraPlot(data);
  specplot->SaveCanvas(specplot->GetName());
  auto turnonplot = MakeTurnonPlot(data);
  turnonplot->SaveCanvas(turnonplot->GetName());
  auto ratioplot = MakeRatioPlot(data);
  ratioplot->SaveCanvas(ratioplot->GetName());
}