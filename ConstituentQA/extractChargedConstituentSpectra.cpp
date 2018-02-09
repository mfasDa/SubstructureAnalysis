#ifndef __CLING__
#include <memory>
#include <vector>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TROOT.h>
#endif

#include "../helpers/graphics.C"

const double kVerySmall = 1e-5;

struct PtBin{
  double  fMin;
  double  fMax;
  TH1     *fSpectrum;
};

struct JetRData {
  THnSparse *fSparseJet;
  THnSparse *fSparseCharged;
  double r;
};

TH1 *extractConstituentSpectrum(THnSparse *dataCharged, const char *observable, double jetptmin, double jetptmx) {
  // set cuts
  auto obsbin = -1;
  if(!strcmp(observable, "ptch")) obsbin = 4;
  else if(!strcmp(observable, "zch")) obsbin = 5;

  auto globmin = dataCharged->GetAxis(0)->GetFirst(), globmax = dataCharged->GetAxis(0)->GetLast(),
       binptmin = dataCharged->GetAxis(0)->FindBin(jetptmin + kVerySmall), binptmax = dataCharged->GetAxis(0)->FindBin(jetptmx - kVerySmall);
  dataCharged->GetAxis(0)->SetRange(binptmin, binptmax);

  auto projected = dataCharged->Projection(obsbin);
  projected->SetName(Form("%s_%d_%d", observable, int(jetptmin), int(jetptmx)));
  projected->SetDirectory(nullptr);

  // reset range
  dataCharged->GetAxis(0)->SetRange(globmin, globmax);
  return projected;
}

std::vector<PtBin> extractConstituentSpectraRaw(THnSparse *dataCharged, const char *observable) {
  std::vector<PtBin> ptbins;
  for(auto b = 20.; b < 200.; b+= 20.) ptbins.push_back({b, b+20., nullptr});

  // Make cut on Neutral energy fraction (0.02 ... 0.98)
  int globmin = dataCharged->GetAxis(1)->GetFirst(), globmax = dataCharged->GetAxis(1)->GetLast(),
      binNEFmin = dataCharged->GetAxis(1)->FindBin(0.02 + kVerySmall), binNEFmax = dataCharged->GetAxis(1)->FindBin(0.98 - kVerySmall);
  dataCharged->GetAxis(1)->SetRange(binNEFmin, binNEFmax);
  for(auto &b : ptbins){
    b.fSpectrum = extractConstituentSpectrum(dataCharged, observable, b.fMin, b.fMax);
  }
  dataCharged->GetAxis(1)->SetRange(globmin, globmax);
  return ptbins;
}

double GetJetYield(THnSparse *dataJet, double jetptmin, double jetptmax) {
  // Make cut on Neutral energy fraction (0.02 ... 0.98)
  int globmin = dataJet->GetAxis(1)->GetFirst(), globmax = dataJet->GetAxis(1)->GetLast(),
      binNEFmin = dataJet->GetAxis(1)->FindBin(0.02 + kVerySmall), binNEFmax = dataJet->GetAxis(1)->FindBin(0.98 - kVerySmall); 
  
  auto jetpthist = std::unique_ptr<TH1>(dataJet->Projection(0));
  auto njet = jetpthist->Integral(jetpthist->GetXaxis()->FindBin(jetptmin + kVerySmall), jetpthist->GetXaxis()->FindBin(jetptmax - kVerySmall));
  dataJet->GetAxis(1)->SetRange(globmin, globmax);
  return njet;
}

std::vector<PtBin> GetNormalizedSpectra(JetRData &histos, const char *observable) {
  auto spectra = extractConstituentSpectraRaw(histos.fSparseCharged, observable);
  for(auto &spec: spectra) {
    spec.fSpectrum->Scale(1./GetJetYield(histos.fSparseJet, spec.fMin, spec.fMax));
  }
  return spectra; 
}

JetRData ExtractTHnSparses(TFile &datafile, const char *trigger, double r) {
  datafile.cd(Form("JetConstituentQA_%s", trigger));
  auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
  auto jetsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hJetCounterfulljets_R%02d", int(r*10))));
  auto chargedsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hChargedConstituentsfulljets_R%02d", int(r*10.))));
  return {jetsparse, chargedsparse, r};
}

void extractChargedConstituentSpectra(const char *datafile, const char *trigger){
  auto datareader = std::unique_ptr<TFile>(TFile::Open(datafile, "READ"));
  gROOT->cd();
  std::map<double, JetRData> datasets;
  for(auto r  = 0.2; r <= 0.4; r += 0.2) datasets[r] = ExtractTHnSparses(*datareader, trigger, r);
  datareader->Close();

  std::map<double, Style> styles = {{20., {kRed, 24}}, {40., {kBlue, 25}}, {60., {kGreen, 26}}, {80., {kViolet, 27}}, {100., {kOrange, 28}}, {120., {kTeal, 29}}, {140., {kMagenta, 30}}, {160., {kGray, 24}}, {180., {kBlack, 25}}};

  auto plot = new TCanvas("spectraPlot", "spectaPlot", 1000, 800);
  plot->Divide(2, 2);

  int row  = 0; 
  TLegend *leg(nullptr);
  for(auto r : datasets) {
    plot->cd(2*row + 1);
    gPad->SetLogy();
    double ptframe[] =  {0., 200., 1e-9, 1000.};
    make_frame(Form("ptch_R%02d", int(r.first)), "p_{t,ch} (GeV/c)", "1/N_{jet} dN_{ch}/dp_{t,ch} ((GeV/c)^{-1})", ptframe)->Draw("axis");
    if(row == 0) {
      leg = new TLegend(0.1, 0.65, 0.89, 0.89);
      InitWidget<TLegend>(*leg);
      leg->SetNColumns(3);
      leg->SetTextSize(0.019);
      leg->Draw();
    }
    for(auto s : GetNormalizedSpectra(r.second, "ptch")){
      auto spec = s.fSpectrum;
      styles.find(s.fMin)->second.SetStyle(*spec);
      spec->Draw("epsame");
      if(row == 0){
        leg->AddEntry(spec, Form("%.1f GeV/c < p_{t,j} < %.1f GeV/c", s.fMin, s.fMax), "lep");
      }
    }

    plot->cd(2*row + 2);
    gPad->SetLogy();
    double zframe[] = {0., 1., 1e-6, 100.};
    make_frame(Form("zch_R%02d", int(r.first)), "z_{ch}", "1/N_{jet} dN_{ch}/dz_{ch}", zframe)->Draw("axis");
    auto label = new TPaveText(0.15, 0.82, 0.35, 0.89, "NDC");
    InitWidget<TPaveText>(*label);
    label->AddText(Form("Full jets, R=%0.1f", r.first));
    label->Draw();
    for(auto s  : GetNormalizedSpectra(r.second, "zch")){
      auto spec = s.fSpectrum;
      styles.find(s.fMin)->second.SetStyle(*spec);
      spec->Draw("epsame");
    }
    row++;
  }
  plot->cd();
  plot->Update();
}