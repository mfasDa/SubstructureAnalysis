#ifndef __CLING__
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TF1.h>
#include <TH1.h>
#include <TLegend.h>
#endif

#include "../helpers/graphics.C"

void makeRejectionJetPatches(const std::string_view inputfile) {
  auto reader = std::unique_ptr<TFile>(TFile::Open(inputfile.data(), "READ"));
  auto specMB = static_cast<TH1 *>(reader->Get("hPatchADCEJEMB")),
       specEJ1 = static_cast<TH1 *>(reader->Get("hPatchADCEJEEJ1ANY")),
       specEJ2 = static_cast<TH1 *>(reader->Get("hPatchADCEJEEJ2ANY"));

  specMB->SetDirectory(nullptr);
  specEJ1->SetDirectory(nullptr);
  specEJ2->SetDirectory(nullptr);

  specMB->Rebin(16);
  specEJ1->Rebin(16);
  specEJ2->Rebin(16);

  auto mbstyle = Style{kBlack, 20}, ej1style = Style{kRed, 24}, ej2style = Style{kBlue, 25};
  mbstyle.SetStyle<TH1>(*specMB);
  ej1style.SetStyle<TH1>(*specEJ1);
  ej2style.SetStyle<TH1>(*specEJ2);

  auto ratJ1J2 = static_cast<TH1 *>(specEJ1->Clone("ratEJ1EJ2")),
       ratJ2MB = static_cast<TH1 *>(specEJ2->Clone("ratEJ2MB"));

  ratJ1J2->SetDirectory(nullptr);
  ratJ1J2->Divide(specEJ2);
  ej1style.SetStyle<TH1>(*ratJ1J2);
  ratJ2MB->SetDirectory(nullptr);
  ratJ2MB->Divide(specMB);
  ej2style.SetStyle<TH1>(*ratJ2MB);

  auto plot = new TCanvas("plot", "Patch turnon jet trigger", 1000, 500);
  plot->Divide(2,1);

  plot->cd(1);
  gPad->SetLogy();
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto specframe = new TH1F("specframe", "; ADC; 1/N_{trg} dN/dADC", 1000, 0., 1000);
  specframe->SetDirectory(nullptr);
  specframe->SetStats(false);
  specframe->GetYaxis()->SetRangeUser(1e-6, 10000);
  specframe->Draw("axis");

  auto leg = new TLegend(0.65, 0.7, 0.89, 0.89);
  InitWidget<TLegend>(*leg);
  leg->Draw();

  specMB->Draw("epsame");
  leg->AddEntry(specMB, "MB", "leg");
  specEJ1->Draw("epsame");
  leg->AddEntry(specEJ1, "EJ1", "lep");
  specEJ2->Draw("epsame");
  leg->AddEntry(specEJ2, "EJ2", "lep");

  plot->cd(2);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto ratioframe = new TH1F("ratioframe", "; ADC; ratio triggers", 1000, 0., 1000);
  ratioframe->SetDirectory(nullptr);
  ratioframe->SetStats(false);
  ratioframe->GetYaxis()->SetRangeUser(0., 2000.);
  ratioframe->Draw("axis");

  ratJ1J2->Draw("epsame");
  ratJ2MB->Draw("epsame");

  auto ratleg = new TLegend(0.15, 0.8, 0.77, 0.89);
  InitWidget<TLegend>(*ratleg);
  ratleg->Draw("epsame");

  TF1 *highfit = new TF1("highfit", "pol0", 0., 1000.), *lowfit = new TF1("lowfit", "pol0", 0., 1000.);
  ratJ1J2->Fit(highfit, "N", "", 300, 600);
  highfit->SetLineColor(ej1style.color);
  highfit->SetLineStyle(2);
  highfit->Draw("lsame");
  ratleg->AddEntry(ratJ1J2, Form("EJ1/EG2: %.3f #pm %.3f", highfit->GetParameter(0), highfit->GetParError(0)), "lep");
  ratJ2MB->Fit(lowfit, "N", "", 300, 600);
  lowfit->SetLineColor(ej2style.color);
  lowfit->SetLineStyle(2);
  lowfit->Draw("lsame");
  ratleg->AddEntry(ratJ2MB, Form("EJ2/MB: %.3f #pm %.3f", lowfit->GetParameter(0), lowfit->GetParError(0)), "lep");

  plot->cd();
  plot->Update();

  std::array<std::string, 5> filetypes = {{"eps", "pdf", "png", "jpeg", "gif"}};
  for(auto f : filetypes) {
    std::stringstream outfilename;
    outfilename << "rejectionEJEpatches." << f;
    plot->SaveAs(outfilename.str().data());
  }
}
