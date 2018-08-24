#ifndef __CLING__
#include <ROOT/RDataFrame.hxx>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/substructuretree.C"

std::vector<double> getZgBinningFine(){
  std::vector<double> result;
  result.push_back(0.);
  for(double d = 0.1; d <= 0.5; d += 0.05) result.push_back(d);
  return result;
}

std::vector<double> getMinBiasPtBinningRealistic() {
  std::vector<double> result;
  double current = 20;
  result.push_back(current);
  while(current < 30.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 40.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 60.) {
    current += 10.;
    result.push_back(current);
  }
  std::cout << "Using binning for zg0 test" << std::endl;
  while(current < 120.) {
    current += 10.;
    result.push_back(current);
  }

  return result;
}

std::vector<double> getMinBiasPtBinningPart() {
  std::vector<double> result;
  double current = 0;
  result.push_back(current);
  while(current < 20.) {
    current += 20.;
    result.push_back(current);
  }
  while(current < 40.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 240.) {
    current += 10.;
    result.push_back(current);
  }
  return result;
}

void InspectZgBin0(const std::string_view filename, const std::string_view trigger = "INT7"){
  ROOT::EnableImplicitMT(8);
  auto tag = getFileTag(filename);
  auto jd = getJetType(tag);
  auto ptbinningPart = getPtBinningPart(trigger), ptbinningdet = getPtBinningRealistic(trigger), zgbinning = getZgBinningFine(),
       zgbinningfine = makeLinearBinning(25, 0., 0.5), binningptfine = makeLinearBinning(40, 0., 200.);
  auto ptdiffbinning = makeLinearBinning(100, -1., 1.);
  double ptdetmin = *ptbinningdet.begin(), ptdetmax = *ptbinningdet.rbegin(),
         ptpartmin = *ptbinningPart.begin(), ptpartmax = *ptbinningPart.rbegin();
  std::string filterptdet = Form("PtJetRec >= %f && PtJetRec < %f", ptdetmin, ptdetmax);

  ROOT::RDataFrame zgframe(GetNameJetSubstructureTree(filename), filename);
  // define datasets
  auto data_truncated = zgframe.Filter(filterptdet);
  auto zgpart0 = data_truncated.Filter("ZgTrue < 0.1");
  auto zgdet0 = data_truncated.Filter("ZgMeasured < 0.1");
  // non-truncated datasets
  auto zgpart0_nontrunc = zgframe.Filter("ZgTrue < 0.1");
  auto zgdet0_nontrunc = zgframe.Filter("ZgMeasured < 0.1");
  auto data_ptdiff = zgframe.Define("PtDiff", "(PtJetRec - PtJetSim)/PtJetRec");

  // control histograms: pt distributions with/without truncation
  auto hist_ptrec_nontrunc = zgframe.Histo1D({"hist_ptrec_nontrunc", "p_{t,jet,rec} non-truncated; p_{t,jet} (GeV/c); d#sigma/dp_{t}", 200, 0., 200.}, "PtJetRec", "PythiaWeight");
  auto hist_ptrec_trunc = data_truncated.Histo1D({"hist_ptrec_nontrunc", "p_{t,jet,rec} non-truncated; p_{t,jet} (GeV/c); d#sigma/dp_{t}", 200, 0., 200.}, "PtJetRec", "PythiaWeight");

  // zg distributions at det / part level
  auto hist_allzg_ptdet = zgframe.Histo1D({"hist_zgdet_ptdet", "p_{t,det} spectrum for all jets}; p_{t, det}", static_cast<int>(ptbinningdet.size())-1, ptbinningdet.data()}, "PtJetRec", "PythiaWeight");
  auto hist_allzg_ptpart_nontruncated = zgframe.Histo1D({"hist_zgdet_ptdet", "p_{t,part} spectrum for all jets}; p_{t, det}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");
  auto hist_allzg_ptpart_truncated = data_truncated.Histo1D({"hist_zgdet_ptdet", "p_{t,part} spectrum for all jets}; p_{t, det}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");
  auto hist_zgdet0_ptdet = zgdet0.Histo1D({"hist_zgdet0_ptdet", "p_{t,det}(z_{g,det} = 0) ; p_{t, det} (GeV/c); d#sigma/dp_{t}", static_cast<int>(ptbinningdet.size())-1, ptbinningdet.data()}, "PtJetRec", "PythiaWeight");
  auto hist_zgdet0_ptpart_truncated = zgdet0.Histo1D({"hist_zgdet0_ptpart", "p_{t, part}(z_{g,det} = 0); p_{t,part} (GeV/c); dsigma/dp_{t}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");
  auto hist_zgpart0_ptpart_truncated = zgpart0.Histo1D({"hist_zgpart0_ptpart", "p_{t, part}(z_{g,det} = 0); p_{t,part} (GeV/c); dsigma/dp_{t}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");
  auto hist_zgdet0_ptpart_nontruncated = zgdet0.Histo1D({"hist_zgdet0_ptpart", "p_{t, part}(z_{g,det} = 0); p_{t,part} (GeV/c); dsigma/dp_{t}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");
  auto hist_zgpart0_ptpart_nontruncated = zgpart0.Histo1D({"hist_zgpart0_ptpart", "p_{t, part}(z_{g,det} = 0); p_{t,part} (GeV/c); dsigma/dp_{t}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetSim", "PythiaWeight");

  auto hist_pt_det_part = zgframe.Histo2D({"hist_det_part", "p_{t,det} vs. p_{t,part}; p_{t,det} (GeV/c); p_{t,part} (GeV/c)", static_cast<int>(ptbinningdet.size())-1, ptbinningdet.data(), static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data()}, "PtJetRec", "PtJetSim", "PythiaWeight");
  auto hist_pt_det_part_fine = zgframe.Histo2D({"hist_det_part_fine", "p_{t,det} vs. p_{t,part}; p_{t,det} (GeV/c); p_{t,part} (GeV/c)", static_cast<int>(binningptfine.size())-1, binningptfine.data(), static_cast<int>(binningptfine.size())-1, binningptfine.data()}, "PtJetRec", "PtJetSim", "PythiaWeight");
  auto hist_diff_pt_det_part = data_ptdiff.Histo2D({"hist_diff_pt_det_part", "jet p_{t} response; p_{t,part} (GeV/c); (p_{t,det} - p_{t,part})/p_{t,part}", static_cast<int>(ptbinningPart.size())-1, ptbinningPart.data(), static_cast<int>(ptdiffbinning.size())-1, ptdiffbinning.data()}, "PtJetSim", "PtDiff", "PythiaWeight");
  auto hist_diff_pt_det_part_fine = data_ptdiff.Histo2D({"hist_diff_pt_det_part_fine", "jet p_{t} response; p_{t,part} (GeV/c); (p_{t,det} - p_{t,part})/p_{t,part}", static_cast<int>(binningptfine.size())-1, binningptfine.data(), static_cast<int>(ptdiffbinning.size())-1, ptdiffbinning.data()}, "PtJetSim", "PtDiff", "PythiaWeight");

  // Response matrices
  std::vector<ROOT::RDF::RResultPtr<TH2D>> responsematrices, responsematricesfine;
  for(auto ptbin : ROOT::TSeqI(0, ptbinningPart.size() -1)){
    std::string ptpartfilter = Form("PtJetSim >= %f && PtJetSim < %f", ptbinningPart[ptbin], ptbinningPart[ptbin+1]);
    responsematrices.emplace_back(data_truncated.Filter(ptpartfilter).Histo2D({Form("responsezg_%d_%d", int(ptbinningPart[ptbin]), int(ptbinningPart[ptbin+1])), 
                                                                               Form("%.1f GeV/c < p_{t,part} < %.1f GeV/c; z_{g,det}; z_{g,part}", ptbinningPart[ptbin], ptbinningPart[ptbin+1]), 
                                                                               static_cast<int>(zgbinning.size())-1, zgbinning.data(), static_cast<int>(zgbinning.size())-1, zgbinning.data()}, 
                                                                               "ZgMeasured", "ZgTrue", "PythiaWeight"));
    responsematricesfine.emplace_back(data_truncated.Filter(ptpartfilter).Histo2D({Form("responsefinezg_%d_%d", int(ptbinningPart[ptbin]), int(ptbinningPart[ptbin+1])), 
                                                                               Form("%.1f GeV/c < p_{t,part} < %.1f GeV/c; z_{g,det}; z_{g,part}", ptbinningPart[ptbin], ptbinningPart[ptbin+1]), 
                                                                               static_cast<int>(zgbinningfine.size())-1, zgbinningfine.data(), static_cast<int>(zgbinningfine.size())-1, zgbinningfine.data()}, 
                                                                               "ZgMeasured", "ZgTrue", "PythiaWeight"));
  }
  
  // Response matrices in pt
  std::vector<ROOT::RDF::RResultPtr<TH2D>> responsematricespt, responsematricesptfine;
  for(auto zgbin : ROOT::TSeqI(0, zgbinning.size()-1)){
    std::string selectionstring = Form("ZgTrue >= %.2f && ZgTrue < %.2f", zgbinning[zgbin], zgbinning[zgbin+1]);
    responsematricespt.emplace_back(zgframe.Filter(selectionstring).Histo2D({Form("responsept_%02d_%02d", int(zgbinning[zgbin]*100), int(zgbinning[zgbin+1]*100)), 
                                                                             Form("%.1f < z_{g,part} < %.1f; p_{t,det} (GeV/c); p_{t,part} (GeV/c)", zgbinning[zgbin], zgbinning[zgbin+1]), static_cast<int>(ptbinningdet.size())-1, ptbinningdet.data(), static_cast<int>(ptbinningPart.size()-1), ptbinningPart.data()}, 
                                                                             "PtJetRec", "PtJetSim", "PythiaWeight"));
    responsematricesptfine.emplace_back(zgframe.Filter(selectionstring).Histo2D({Form("responsefinept_%02d_%02d", int(zgbinning[zgbin]*100), int(zgbinning[zgbin+1]*100)), 
                                                                             Form("%.1f < z_{g,part} < %.1f; p_{t,det} (GeV/c); p_{t,part} (GeV/c)", zgbinning[zgbin], zgbinning[zgbin+1]), static_cast<int>(binningptfine.size())-1, binningptfine.data(), static_cast<int>(binningptfine.size()-1), binningptfine.data()}, 
                                                                             "PtJetRec", "PtJetSim", "PythiaWeight"));
  }

  // Calculate fraction zg per pt-bin
  auto hist_frac_zg0_det = new TH1D(*hist_zgdet0_ptdet),
       hist_frac_zg0_part_nontruncated = new TH1D(*hist_zgpart0_ptpart_nontruncated),
       hist_frac_zgdet0_part_nontruncated = new TH1D(*hist_zgdet0_ptpart_nontruncated),
       hist_frac_zg0_part_truncated = new TH1D(*hist_zgpart0_ptpart_truncated),
       hist_frac_zgdet0_part_truncated = new TH1D(*hist_zgdet0_ptpart_truncated);
  hist_frac_zg0_det->SetDirectory(nullptr);
  hist_frac_zg0_det->Divide(hist_allzg_ptdet.GetPtr());
  hist_frac_zg0_part_nontruncated->SetDirectory(nullptr);
  hist_frac_zg0_part_nontruncated->Divide(hist_allzg_ptpart_nontruncated.GetPtr());
  hist_frac_zg0_part_truncated->SetDirectory(nullptr);
  hist_frac_zg0_part_truncated->Divide(hist_allzg_ptpart_truncated.GetPtr());
  hist_frac_zgdet0_part_nontruncated->SetDirectory(nullptr);
  hist_frac_zgdet0_part_nontruncated->Divide(hist_allzg_ptpart_nontruncated.GetPtr());
  hist_frac_zgdet0_part_truncated->SetDirectory(nullptr);
  hist_frac_zgdet0_part_truncated->Divide(hist_allzg_ptpart_truncated.GetPtr());

  Style{kRed, 24}.SetStyle<TH1>(*hist_frac_zg0_det);
  Style{kBlue, 25}.SetStyle<TH1>(*hist_frac_zg0_part_nontruncated);
  Style{kGreen + 2, 26}.SetStyle<TH1>(*hist_frac_zgdet0_part_nontruncated);
  Style{kGray, 28}.SetStyle<TH1>(*hist_frac_zg0_part_truncated);
  Style{kOrange + 2, 29}.SetStyle<TH1>(*hist_frac_zgdet0_part_truncated);

  // Calculate feed-in/feed-out
  auto hist_ratiozg0_detpart_nontruncated = new TH1D(*hist_zgdet0_ptpart_nontruncated);
  hist_ratiozg0_detpart_nontruncated->SetDirectory(nullptr);
  hist_ratiozg0_detpart_nontruncated->Divide(hist_zgpart0_ptpart_nontruncated.GetPtr());
  auto hist_ratiozg0_detpart_truncated = new TH1D(*hist_zgdet0_ptpart_truncated);
  hist_ratiozg0_detpart_truncated->SetDirectory(nullptr);
  hist_ratiozg0_detpart_truncated->Divide(hist_zgpart0_ptpart_truncated.GetPtr());
  Style{kViolet, 26}.SetStyle<TH1>(*hist_ratiozg0_detpart_nontruncated);
  Style{kCyan, 27}.SetStyle<TH1>(*hist_ratiozg0_detpart_truncated);

  // Plot
  auto plotptcontrol = new ROOT6tools::TSavableCanvas(Form("ptrectrunc_%s", tag.data()), "truncation control plot", 800, 600);
  plotptcontrol->cd();
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("trunccontrolframe", "p_{t,jet,rec} (GeV/c)", "d#sigma/dp_{t} (mb/(GeV/c))", 0., 200, 1e-9, 1.))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.6, 0.89, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
  auto legpt = new ROOT6tools::TDefaultLegend(0.55, 0.7, 0.89, 0.89);
  legpt->Draw();
  Style{kRed, 24}.SetStyle<TH1>(*hist_ptrec_nontrunc);
  Style{kBlue, 25}.SetStyle<TH1>(*hist_ptrec_trunc);
  hist_ptrec_nontrunc->DrawClone("epsame");
  hist_ptrec_trunc->DrawClone("epsame");
  legpt->AddEntry(hist_ptrec_nontrunc.GetPtr(), "Not truncated", "lep");
  legpt->AddEntry(hist_ptrec_trunc.GetPtr(), "Truncated", "lep");
  plotptcontrol->SaveCanvas(plotptcontrol->GetName());

  auto ptresplot = new ROOT6tools::TSavableCanvas(Form("ptres_%s", tag.data()), "Pt-resolution", 1200, 600);
  ptresplot->Divide(2,1);

  ptresplot->cd(1);
  gPad->SetLogz();
  hist_pt_det_part->SetStats(false);
  hist_pt_det_part->DrawCopy("colz");
  (new ROOT6tools::TNDCLabel(0., 0., 0.65, 0.07, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw("axis");

  ptresplot->cd(2);
  gPad->SetLogz();
  hist_diff_pt_det_part->SetStats(false);
  hist_diff_pt_det_part->DrawCopy("colz");

  ptresplot->cd();
  ptresplot->SaveCanvas(ptresplot->GetName());
  
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
  auto leg = new ROOT6tools::TDefaultLegend(0.55, 0.65, 0.89, 0.89);
  leg->Draw();
  hist_frac_zg0_part_nontruncated->Draw("epsame");
  hist_frac_zgdet0_part_nontruncated->Draw("epsame");
  hist_frac_zg0_part_truncated->Draw("epsame");
  hist_frac_zgdet0_part_truncated->Draw("epsame");
  leg->AddEntry(hist_frac_zg0_part_nontruncated, "z_{g,part} = 0, not truncated", "lep");
  leg->AddEntry(hist_frac_zgdet0_part_nontruncated, "z_{g,det} = 0, not truncated", "lep");
  leg->AddEntry(hist_frac_zg0_part_truncated, "z_{g,part} = 0, truncated", "lep");
  leg->AddEntry(hist_frac_zgdet0_part_truncated, "z_{g,det} = 0, truncated", "lep");

  plotzg0->cd(3);
  gPad->SetLeftMargin(0.15);
  (new ROOT6tools::TAxisFrame("zg0feedframe", "p_{t, part} (GeV/c", "N(z_{g,det} = 0)/N(z_{g,part} = 0)", ptpartmin, ptpartmax, 0., 10.))->Draw("axis");
  auto legrat  = new ROOT6tools::TDefaultLegend(0.15, 0.7, 0.4, 0.89);
  legrat->Draw();
  hist_ratiozg0_detpart_nontruncated->Draw("epsame");
  hist_ratiozg0_detpart_truncated->Draw("epsame");
  legrat->AddEntry(hist_ratiozg0_detpart_nontruncated, "Not truncated", "lep");
  legrat->AddEntry(hist_ratiozg0_detpart_truncated, "Truncated", "lep");

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
  hist_allzg_ptpart_nontruncated->Write();
  hist_allzg_ptpart_truncated->Write();
  hist_zgdet0_ptdet->Write();
  hist_zgdet0_ptpart_nontruncated->Write();
  hist_zgdet0_ptpart_truncated->Write();
  hist_zgpart0_ptpart_nontruncated->Write();
  hist_zgpart0_ptpart_truncated->Write();
  hist_frac_zg0_det->Write();
  hist_frac_zg0_part_nontruncated->Write();
  hist_frac_zg0_part_truncated->Write();
  hist_ratiozg0_detpart_nontruncated->Write();
  hist_ratiozg0_detpart_truncated->Write();
  hist_pt_det_part->Write();
  hist_pt_det_part_fine->Write();
  hist_diff_pt_det_part->Write();
  hist_diff_pt_det_part_fine->Write();
  for(auto resp : responsematrices) resp->Write();
  for(auto resp : responsematricesfine) resp->Write();
  for(auto resp : responsematricespt) resp->Write();
  for(auto resp : responsematricesptfine) resp->Write();
}