#ifndef __CLING__
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <RStringView.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#endif

#include "../helpers/graphics.C"

void makeRejectionGammaPatches(const std::string_view inputfile) {
  auto reader = std::unique_ptr<TFile>(TFile::Open(inputfile.data(), "READ"));
  auto specMB = static_cast<TH1 *>(reader->Get("hPatchADCEGAMB")),
       specEG1 = static_cast<TH1 *>(reader->Get("hPatchADCEGAEG1ANY")),
       specEG2 = static_cast<TH1 *>(reader->Get("hPatchADCEGAEG2ANY"));

  specMB->Rebin(16);
  specEG1->Rebin(16);
  specEG2->Rebin(16);

  specMB->SetDirectory(nullptr);
  specEG1->SetDirectory(nullptr);
  specEG2->SetDirectory(nullptr);

  auto mbstyle = Style{kBlack, 20}, eg1style = Style{kRed, 24}, eg2style = Style{kBlue, 25};
  mbstyle.SetStyle<TH1>(*specMB);
  eg1style.SetStyle<TH1>(*specEG1);
  eg2style.SetStyle<TH1>(*specEG2);

  auto ratG1G2 = static_cast<TH1 *>(specEG1->Clone("ratEG1EG2")),
       ratG2MB = static_cast<TH1 *>(specEG2->Clone("ratEG2MB"));

  ratG1G2->SetDirectory(nullptr);
  ratG1G2->Divide(specEG2);
  eg1style.SetStyle<TH1>(*ratG1G2);
  ratG2MB->SetDirectory(nullptr);
  ratG2MB->Divide(specMB);
  eg2style.SetStyle<TH1>(*ratG2MB);

  auto plot = new TCanvas("plot", "Patch turnon gamma trigger", 1000, 500);
  plot->Divide(2,1);

  plot->cd(1);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  gPad->SetLogy();
  auto specframe = new TH1F("specframe", "; ADC; 1/N_{trg} dN/dADC", 1000, 0., 1000);
  specframe->SetDirectory(nullptr);
  specframe->SetStats(false);
  specframe->GetYaxis()->SetRangeUser(1e-6, 1000);
  specframe->Draw("axis");

  auto leg = new TLegend(0.65, 0.7, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->Draw();

  specMB->Draw("epsame");
  leg->AddEntry(specMB, "MB", "leg");
  specEG1->Draw("epsame");
  leg->AddEntry(specEG1, "EG1", "lep");
  specEG2->Draw("epsame");
  leg->AddEntry(specEG2, "EG2", "lep");

  plot->cd(2);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto ratioframe = new TH1F("ratioframe", "; ADC; ratio triggers", 1000, 0., 1000);
  ratioframe->SetDirectory(nullptr);
  ratioframe->SetStats(false);
  ratioframe->GetYaxis()->SetRangeUser(0., 800.);
  ratioframe->Draw("axis");

  ratG1G2->Draw("epsame");
  ratG2MB->Draw("epsame");

  auto ratleg = new TLegend(0.15, 0.8, 0.77, 0.89);
  InitWidget<TLegend>(*ratleg);
  ratleg->Draw("epsame");

  TF1 *highfit = new TF1("highfit", "pol0", 0., 600.), *lowfit = new TF1("lowfit", "pol0", 0., 600.);
  ratG1G2->Fit(highfit, "N", "", 150, 600);
  highfit->SetLineColor(eg1style.color);
  highfit->SetLineStyle(2);
  highfit->Draw("lsame");
  ratleg->AddEntry(ratG1G2, Form("EG1/EG2: %.3f #pm %.3f", highfit->GetParameter(0), highfit->GetParError(0)), "lep");
  ratG2MB->Fit(lowfit, "N", "", 100, 400);
  lowfit->SetLineColor(eg2style.color);
  lowfit->SetLineStyle(2);
  lowfit->Draw("lsame");
  ratleg->AddEntry(ratG2MB, Form("EG2/MB: %.3f #pm %.3f", lowfit->GetParameter(0), lowfit->GetParError(0)), "lep");

  plot->cd();
  plot->Update();

  std::array<std::string, 5> filetypes = {{"eps", "pdf", "png", "jpeg", "gif"}};
  for(auto f : filetypes) {
    std::stringstream outfilename;
    outfilename << "rejectionEGApatches." << f;
    plot->SaveAs(outfilename.str().data());
  }
}
