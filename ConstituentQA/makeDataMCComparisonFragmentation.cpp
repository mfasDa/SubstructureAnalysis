#ifndef __CLING__
#include <memory>
#include <ROOT/TSeq.hxx>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#include <TPaveText.h>
#include <TString.h>
#endif

#include "../helpers/graphics.C"

struct JetData{
  std::unique_ptr<THnSparse>      fJetSparse;
  std::unique_ptr<THnSparse>      fChargedConstituents;
  std::unique_ptr<THnSparse>      fNeutralConstituents;
};

const double kVerySmall = 1e-5;

enum FragmentationType_t {
  kChargedPt,
  kChargedZ,
  kNeutralE,
  kNeutralZ
};

Double_t GetNumberOfJets(THnSparse *jetsparse, double ptmin, double ptmax){
  auto globneffirst = jetsparse->GetAxis(1)->GetFirst(), globneflast = jetsparse->GetAxis(1)->GetLast(),
       binNefMin = jetsparse->GetAxis(1)->FindBin(0.02 + kVerySmall), binNefMax = jetsparse->GetAxis(1)->FindBin(0.98 - kVerySmall);
  jetsparse->GetAxis(1)->SetRangeUser(binNefMin, binNefMax);
  auto ptdist = std::unique_ptr<TH1>(jetsparse->Projection(0));
  jetsparse->GetAxis(1)->SetRangeUser(globneffirst, globneflast);

  auto binptmin = ptdist->GetXaxis()->FindBin(ptmin + kVerySmall), binptmax = ptdist->GetXaxis()->FindBin(ptmax - kVerySmall);
  return ptdist->Integral(binptmin, binptmax);
}

TH1 * GetFragmentation(JetData &histos, FragmentationType_t frag, Double_t ptmin, Double_t ptmax) {
  THnSparse *constituentsparse = nullptr;
  auto axis = -1;

  switch(frag){
    case kChargedPt: axis = 4; constituentsparse = histos.fChargedConstituents.get(); break;
    case kChargedZ:  axis = 5; constituentsparse = histos.fChargedConstituents.get(); break;
    case kNeutralE:  axis = 4; constituentsparse = histos.fNeutralConstituents.get(); break;
    case kNeutralZ:  axis = 6; constituentsparse = histos.fNeutralConstituents.get(); break;
  }

  // apply NEF and pt cut
  auto globptfirst = constituentsparse->GetAxis(0)->GetFirst(), globptlast = constituentsparse->GetAxis(0)->GetLast(),
       globneffirst = constituentsparse->GetAxis(1)->GetFirst(), globneflast = constituentsparse->GetAxis(1)->GetLast(),
       binptmin = constituentsparse->GetAxis(0)->FindBin(ptmin + kVerySmall), binptmax = constituentsparse->GetAxis(0)->FindBin(ptmax - kVerySmall),
       binnefmin = constituentsparse->GetAxis(1)->FindBin(0.02 + kVerySmall), binnefmax = constituentsparse->GetAxis(1)->FindBin(0.98-kVerySmall);
  constituentsparse->GetAxis(0)->SetRange(binptmin, binptmax);
  constituentsparse->GetAxis(1)->SetRange(binnefmin, binnefmax);
  auto result = constituentsparse->Projection(axis);
  constituentsparse->GetAxis(0)->SetRangeUser(globptfirst, globptlast);
  constituentsparse->GetAxis(1)->SetRange(globneffirst, globneflast);  
  result->SetDirectory(nullptr);
  result->Scale(1./GetNumberOfJets(histos.fJetSparse.get(), ptmin, ptmax));
  return result;
};

JetData Read(const char *filename, const char *trigger, double jetradius){
  auto filereader = std::unique_ptr<TFile>(TFile::Open(filename, "READ"));
  auto dirname = std::string("JetConstituentQA_") + trigger;
  filereader->cd(dirname.data());
  auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
  auto jetsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hJetCounterfulljets_R%02d", int(jetradius*10.)))),
       chargedsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hChargedConstituentsfulljets_R%02d", int(jetradius * 10.)))),
       neutralsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hNeutralConstituentsfulljets_R%02d", int(jetradius * 10.))));
  return {std::unique_ptr<THnSparse>(jetsparse), std::unique_ptr<THnSparse>(chargedsparse), std::unique_ptr<THnSparse>(neutralsparse)};
}

TCanvas *makeFragmentationPlot(JetData &data, JetData &mc, FragmentationType_t t, const char *trigger, double jetradius) {
  std::string tag, xtitle, ytitle;
  double xmax(-1);
  switch(t){
    case kChargedPt: tag = "ptch"; xtitle = "p_{t,ch} (GeV/c)"; ytitle = "1/N_{jet} dN/dp_{t,ch} ((GeV/c)^{-1})"; xmax = 200.; break;
    case kChargedZ: tag = "zch"; xtitle = "z_{ch}"; ytitle = "1/N_{jet} dN/dz_{ch} "; xmax = 1.; break;
    case kNeutralE: tag = "ene"; xtitle = "E_{ne} (GeV)"; ytitle = "1/N_{jet} dN/dE_{ne} (GeV^{-1})"; xmax = 200.; break;
    case kNeutralZ: tag = "zne"; xtitle = "z_{ne}"; ytitle = "1/N_{jet} dN/dz_{ne}"; xmax = 1.; break;
  };
  auto plot = new TCanvas(Form("%s_%s_R%02d", tag.data(), trigger, int(jetradius * 10.)), Form("%s, %s, R=%.1f", tag.data(), trigger, jetradius), 1200, 1000);
  plot->Divide(3,3);
  
  Style datastyle{kBlack, 24}, mcstyle{kRed, 25};

  auto current = 1;
  for(auto iptmin : ROOT::TSeqI(20, 200, 20)){
    double ptmin = iptmin, ptmax = iptmin + 20;
    bool doLegend = current == 1;
    plot->cd(current++);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.06);
    gPad->SetLogy();

    auto frame = new TH1F(Form("frame_%s_%s_R%02d_%d_%d", tag.data(), trigger, int(jetradius*10.),  int(ptmin), int(ptmax)), "", 100, 0., xmax);
    frame->SetDirectory(nullptr);
    frame->SetXTitle(xtitle.data());
    frame->SetYTitle(ytitle.data());
    frame->SetStats(false);
    frame->GetYaxis()->SetRangeUser(1e-6, 100);
    frame->Draw("axis");
    
    TLegend *leg(nullptr);
    if(doLegend) {
      leg = new TLegend(0.64, 0.78, 0.93, 0.89);
      InitWidget<TLegend>(*leg);
      leg->Draw();

      auto jetlabel = new TPaveText(0.64, 0.6, 0.93, 0.78, "NDC");
      InitWidget<TPaveText>(*jetlabel);
      jetlabel->AddText(Form("Full jets, R=%.1f, %s", jetradius, trigger));
      jetlabel->Draw();
    }

    auto ptlabel = new TPaveText(0.19, 0.8, 0.59, 0.89, "NDC");
    InitWidget<TPaveText>(*ptlabel);
    ptlabel->AddText(Form("%.1f GeV/c < p_{t,jet} < %.1f GeV/c", ptmin, ptmax));
    ptlabel->Draw();

    auto dataspec = GetFragmentation(data, t, ptmin, ptmax),
         mcspec = GetFragmentation(mc, t, ptmin, ptmax);
    datastyle.SetStyle(*dataspec);
    mcstyle.SetStyle(*mcspec);
    dataspec->Draw("epsame");
    mcspec->Draw("epsame");
    if(doLegend) {
      leg->AddEntry(dataspec, "Data", "lep");
      leg->AddEntry(mcspec, "MC", "lep");
    }
  }
  plot->cd();
  plot->Update();
  return plot;
} 

void makeFragmentationPlots(const char *datafile, const char *mcfile, const char *trigger, double jetradius) {
  auto jetsData =  Read(datafile, trigger, jetradius),
       jetsMC = Read(mcfile, trigger, jetradius);
  makeFragmentationPlot(jetsData, jetsMC, kChargedPt, trigger, jetradius)->SaveAs(Form("CompDataMC_ptch_R%02d_%s.pdf", int(jetradius*10.), trigger));
  makeFragmentationPlot(jetsData, jetsMC, kChargedZ, trigger, jetradius)->SaveAs(Form("CompDataMC_zch_R%02d_%s.pdf", int(jetradius*10.), trigger));
  makeFragmentationPlot(jetsData, jetsMC, kNeutralE, trigger, jetradius)->SaveAs(Form("CompDataMC_ene_R%02d_%s.pdf", int(jetradius*10.), trigger));
  makeFragmentationPlot(jetsData, jetsMC, kNeutralZ, trigger, jetradius)->SaveAs(Form("CompDataMC_zne_R%02d_%s.pdf", int(jetradius*10.), trigger));
}

void makeDataMCComparisonFragmentation(const char *datafile, const char *mcfile){
  std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};
  std::vector<double> jetradii = {0.2, 0.4};
  for(auto r : jetradii){
    for(auto t : triggers) {
      makeFragmentationPlots(datafile, mcfile, t.data(), r);
    }
  }
}