#ifndef __CLING__
#include <array>
#include <memory>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <THnSparse.h>
#include <TLegend.h>
#include <TList.h>
#include <TPaveText.h>
#include <TROOT.h>
#include <TString.h>
#endif

#include "../helpers/graphics.C"

constexpr auto kVerySmall = 1e-5;

enum {
  kJetPt = 0,
  kJetNEF = 1,
  kJetConstCh = 2,
  kJetConstNe = 3
};

struct PtBin {
  Double_t fPtMin;
  Double_t fPtMax;
  TH1 *fSpectrum;
};

TH1 *GetJetNeutralEnergyFraction(THnSparse *jetsparse, double ptmin, double ptmax) {
  constexpr double kNEFCuts[] = {0.02, 0.98};
  auto globptmin = jetsparse->GetAxis(kJetPt)->GetFirst(), globptmax = jetsparse->GetAxis(kJetPt)->GetLast(),
       globnefmin = jetsparse->GetAxis(kJetNEF)->GetFirst(), globnefmax = jetsparse->GetAxis(kJetNEF)->GetLast();

  // apply cuts
  auto nefbinmin = jetsparse->GetAxis(kJetNEF)->FindBin(kNEFCuts[0] + kVerySmall), nefbinmax = jetsparse->GetAxis(kJetNEF)->FindBin(kNEFCuts[1] - kVerySmall),
       ptbinmin = jetsparse->GetAxis(kJetPt)->FindBin(ptmin + kVerySmall), ptbinmax = jetsparse->GetAxis(kJetPt)->FindBin(ptmax - kVerySmall);
  jetsparse->GetAxis(kJetPt)->SetRange(ptbinmin, ptbinmax);
  jetsparse->GetAxis(kJetNEF)->SetRange(nefbinmin, nefbinmax);

  auto projected = jetsparse->Projection(kJetNEF);
  projected->SetName(Form("nef_%d_%d", int(ptmin), int(ptmax)));
  projected->SetDirectory(nullptr);
  projected->Scale(1./projected->Integral());


  // Reset ranges
  jetsparse->GetAxis(kJetPt)->SetRange(globptmin, globptmax);
  jetsparse->GetAxis(kJetNEF)->SetRange(globnefmin, globnefmax);

  return projected;
}

std::vector<PtBin> GetNEFSpectra(THnSparse *jetsparse, const Style &mystyle) {
  std::vector<PtBin> bins;
  for(auto b : ROOT::TSeqI(20, 200, 20)){
    auto jethist = GetJetNeutralEnergyFraction(jetsparse, double(b), double(b)+20.);
    mystyle.SetStyle<TH1>(*jethist);
    bins.push_back({double(b), double(b)+20., jethist});
  } 
  return bins;
}

std::unique_ptr<THnSparse> ReadJetTHnSparse(const char *filename, const char *trigger, double jetradius) {
  std::unique_ptr<THnSparse> jetsparse;
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename, "READ"));
  TString dirname = TString::Format("ConstituentQA_%s", trigger),
          rstring = TString::Format("R%02d", int(jetradius * 10.));
  for(auto keyhandler : TRangeDynCast<TKey>(reader->GetListOfKeys())){
    if(!TString(keyhandler->GetName()).Contains(dirname)) continue;
    reader->cd(keyhandler->GetName());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    gROOT->cd();
    for(auto h : TRangeDynCast<THnSparse>(histlist)) {
      if(!h) continue;
      TString histname(h->GetName());
      if(histname.Contains("hJetCounter") && histname.Contains(rstring)){
        jetsparse = std::unique_ptr<THnSparse>(h);
        break;
      }
    }
    break;
  }
  return jetsparse;
}

TH1 *make_axis(const char *name, const char *xtitle, const char *ytitle, const std::array<double, 4> &range) {
  auto frame = new TH1F(name, "", 100, range[0], range[1]);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(range[2], range[3]);
  return frame;
}

TPaveText *makePtLabel(const std::array<double, 4> &range, double ptmin, double ptmax) {
  auto label = new TPaveText(range[0], range[1], range[2], range[3], "NDC");
  InitWidget<TPaveText>(*label);
  label->AddText(Form("%.1f (GeV/c) < p_{t,j} < %.1f GeV/c", ptmin, ptmax));
  return label;
}

TPaveText *makeRLabel(const std::array<double, 4> &range, double r) {
  auto label = new TPaveText(range[0], range[1], range[2], range[3], "NDC");
  InitWidget<TPaveText>(*label);
  label->AddText(Form("Full jets, R=%.1f", r));
  return label;
}

void extractNeutralEnergyFraction(const char *trgfile, const char *mbfile) {
  constexpr std::array<double, 2> kJetRadii = {{0.2, 0.4}};
  for(auto r : kJetRadii) {
    auto rstring = TString::Format("R%02d", int(r*10.));
    auto plot = new TCanvas(Form("NEFComparison%s", rstring.Data()), Form("Comparison NEF jets R=%0.1f", r), 1000, 800);
    plot->Divide(3,3);
  
    auto INT7bins = GetNEFSpectra(ReadJetTHnSparse(mbfile, "INT7", r).get(), {kBlack, 20}),
         EJ1bins = GetNEFSpectra(ReadJetTHnSparse(trgfile, "EJ1", r).get(), {kRed, 24}),
         EJ2bins = GetNEFSpectra(ReadJetTHnSparse(trgfile, "EJ2", r).get(), {kBlue, 25});
    
    TLegend *leg(nullptr);
    for(int ibin : ROOT::TSeqI(0, INT7bins.size())){
      plot->cd(ibin+1);
      make_axis(Form("axis_pt_%d_%d", int(INT7bins[ibin].fPtMin), int(INT7bins[ibin].fPtMax)), "NEF", "1/N_{jet} dN/dNEF", {{0., 1., 0., .1}})->Draw("axis");
      if(!ibin) {
        leg = new TLegend(0.65, 0.6, 0.89, 0.89);
        InitWidget<TLegend>(*leg);
        leg->Draw();
        makeRLabel({{0.5, 0.48, 0.89, 0.58}}, r)->Draw();
      }
      makePtLabel({{0.15, 0.8, 0.55, 0.89}}, INT7bins[ibin].fPtMin, INT7bins[ibin].fPtMax)->Draw();

      INT7bins[ibin].fSpectrum->Draw("epsame");
      EJ1bins[ibin].fSpectrum->Draw("epsame");
      EJ2bins[ibin].fSpectrum->Draw("epsame");

      if(!ibin) {
        leg->AddEntry(INT7bins[ibin].fSpectrum, "INT7", "lep");
        leg->AddEntry(EJ1bins[ibin].fSpectrum, "EJ1", "lep");
        leg->AddEntry(EJ2bins[ibin].fSpectrum, "EJ2", "lep");
      }
    }
    plot->cd();
    plot->Update();
  }
}