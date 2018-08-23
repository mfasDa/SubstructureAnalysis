#ifndef __CLING__
#include <ROOT/RDataFrame.hxx>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../binnings/binningZg.C"
#include "../../helpers/graphics.C"
#include "../../helpers/substructuretree.C"

void InspectZgBin0(const std::string_view filename, const std::string_view trigger = "INT7"){
  ROOT::EnableImplicitMT(8);
  auto tag = getFileTag(filename);
  auto jd = getJetType(tag);
  auto ptbinningPart = getPtBinningPart(trigger), ptbinningdet = getPtBinningRealistic(trigger), zgbinning = getZgBinningFine();
  double ptdetmin = *ptbinningdet.begin(), ptdetmax = *ptbinningdet.rbegin(),
         ptpartmin = *ptbinningPart.begin(), ptpartmax = *ptbinningPart.rbegin();
  std::string filterptdet = Form("PtJetRec >= %f && PtJetRec < %f", ptdetmin, ptdetmax);

  ROOT::RDataFrame zgframe(GetNameJetSubstructureTree(filename), filename);
  // define datasets
  auto data_truncated = zgframe.Filter(filterptdet);
  auto zgpart0 = data_truncated.Filter("ZgTrue < 0.1");
  auto zgdet0 = data_truncated.Filter("ZgMeasured < 0.1");

  // zg distributions at det / part level
  auto hist_allzg_ptdet = zgframe.Histo1D({"hist_zgdet_ptdet", "p_{t,det} spectrum for all jets}; p_{t, det}", static_cast<int>(ptbinningdet.size())-1, ptbinningdet.data()}, "PtJetRec", "PythiaWeight");
  auto hist_allzg_ptpart = zgframe.Histo1D({"hist_zgdet_ptdet", "p_{t,part} spectrum for all jets}; p_{t, det}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetRec", "PythiaWeight");
  auto hist_zgdet0_ptdet = zgdet0.Histo1D({"hist_zgdet0_ptdet", "p_{t,det}(z_{g,det} = 0) ; p_{t, det} (GeV/c); d#sigma/dp_{t}", static_cast<int>(ptbinningdet.size())-1, ptbinningdet.data()}, "PtJetRec", "PythiaWeight");
  auto hist_zgdet0_ptpart = zgdet0.Histo1D({"hist_zgdet0_ptpart", "p_{t, part}(z_{g,det} = 0); p_{t,part} (GeV/c); dsigma/dp_{t}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");
  auto hist_zgpart0_ptpart = zgpart0.Histo1D({"hist_zgpart0_ptpart", "p_{t, part}(z_{g,det} = 0); p_{t,part} (GeV/c); dsigma/dp_{t}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");

  // Response matrices
  std::vector<ROOT::RDF::RResultPtr<TH2D>> responsematrices;
  for(auto ptbin : ROOT::TSeqI(0, ptbinningPart.size() -1)){
    std::string ptpartfilter = Form("PtJetSim >= %f && PtJetSim < %f", ptbinningPart[ptbin], ptbinningPart[ptbin+1]);
    responsematrices.emplace_back(data_truncated.Filter(ptpartfilter).Histo2D({Form("responsezg_%d_%d", int(ptbinningPart[ptbin]), int(ptbinningPart[ptbin+1])), 
                                                                               Form("%.1f GeV/c < p_{t,part} < %.1f GeV/c; z_{g,det}; z_{g,part}", ptbinningPart[ptbin], ptbinningPart[ptbin+1]), 
                                                                               static_cast<int>(zgbinning.size())-1, zgbinning.data(), static_cast<int>(zgbinning.size())-1, zgbinning.data()}, 
                                                                               "ZgMeasured", "ZgTrue", "PythiaWeight"));
  }

  // Calculate fraction zg per pt-bin
  auto hist_frac_zg0_det = new TH1D(*hist_zgdet0_ptdet),
       hist_frac_zg0_part = new TH1D(*hist_zgpart0_ptpart),
       hist_frac_zgdet0_part = new TH1D(*hist_zgdet0_ptpart);
  hist_frac_zg0_det->SetDirectory(nullptr);
  hist_frac_zg0_det->Divide(hist_allzg_ptdet.GetPtr());
  hist_frac_zg0_part->SetDirectory(nullptr);
  hist_frac_zg0_part->Divide(hist_allzg_ptpart.GetPtr());
  hist_frac_zgdet0_part->SetDirectory(nullptr);
  hist_frac_zgdet0_part->Divide(hist_allzg_ptpart.GetPtr());

  Style{kRed, 24}.SetStyle<TH1>(*hist_frac_zg0_det);
  Style{kBlue, 25}.SetStyle<TH1>(*hist_frac_zg0_part);
  Style{kGreen + 2, 26}.SetStyle<TH1>(*hist_frac_zgdet0_part);

  // Calculate feed-in/feed-out
  auto hist_ratiozg0_detpart = new TH1D(*hist_zgdet0_ptpart);
  hist_ratiozg0_detpart->SetDirectory(nullptr);
  hist_ratiozg0_detpart->Divide(hist_zgpart0_ptpart.GetPtr());
  Style{kViolet, 26}.SetStyle<TH1>(*hist_ratiozg0_detpart);

  // Plot
  auto plotzg0 = new ROOT6tools::TSavableCanvas(Form("zg0checks_%s", tag.data()), "Zg=0 checks", 1200, 600);
  plotzg0->Divide(3,1);

  plotzg0->cd(1);
  gPad->SetLeftMargin(0.15);
  (new ROOT6tools::TAxisFrame("zg0detframe", "p_{t, det} (GeV/c", "N(z_{g,det} = 0)/N(all)", ptdetmin, ptdetmax, 0., 0.1))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.6, 0.89, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
  hist_frac_zg0_det->Draw("epsame");
  
  plotzg0->cd(2);
  gPad->SetLeftMargin(0.15);
  (new ROOT6tools::TAxisFrame("zg0partframe", "p_{t, part} (GeV/c", "N(z_{g} = 0)/N(all)", ptpartmin, ptpartmax, 0., 0.1))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
  leg->Draw();
  hist_frac_zg0_part->Draw("epsame");
  hist_frac_zgdet0_part->Draw("epsame");
  leg->AddEntry(hist_frac_zg0_part, "z_{g,part} = 0", "lep");
  leg->AddEntry(hist_frac_zgdet0_part, "z_{g,det} = 0", "lep");

  plotzg0->cd(3);
  gPad->SetLeftMargin(0.15);
  (new ROOT6tools::TAxisFrame("zg0feedframe", "p_{t, part} (GeV/c", "N(z_{g,det} = 0)/N(z_{g,part} = 0)", ptpartmin, ptpartmax, 0., 10.))->Draw("axis");
  hist_ratiozg0_detpart->Draw("epsame");

  plotzg0->cd();
  plotzg0->SaveCanvas(plotzg0->GetName());

  auto plot2d = new ROOT6tools::TSavableCanvas(Form("zgresponsebins_%s", tag.data()), "2D response matrices", 1200, 1000);
  plot2d->DivideSquare(responsematrices.size());
  for(auto ipad : ROOT::TSeqI(0, responsematrices.size())){
    plot2d->cd(ipad+1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.15);
    gPad->SetLogz();
    responsematrices[ipad]->GetXaxis()->SetTitleSize(0.045);
    responsematrices[ipad]->GetYaxis()->SetTitleSize(0.045);
    responsematrices[ipad]->SetStats(false);
    responsematrices[ipad]->DrawCopy("colz");
    if(ipad == 0) (new ROOT6tools::TNDCLabel(0., 0., 0.65, 0.07, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw("axis");
  }
  plot2d->cd();
  plot2d->SaveCanvas(plot2d->GetName());

  // Write to file
  std::unique_ptr<TFile> writer(TFile::Open(Form("zgresponse_%s.root", tag.data()), "RECREATE"));
  hist_allzg_ptdet->Write();
  hist_allzg_ptpart->Write();
  hist_zgdet0_ptdet->Write();
  hist_zgdet0_ptpart->Write();
  hist_zgpart0_ptpart->Write();
  hist_frac_zg0_det->Write();
  hist_frac_zg0_part->Write();
  hist_ratiozg0_detpart->Write();
  for(auto resp : responsematrices) resp->Write();
}