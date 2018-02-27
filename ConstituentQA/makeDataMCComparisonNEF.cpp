#ifndef __CLING__
#include <memory>
#include <ROOT/TSeq.hxx>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TLegend.h>
#include <TPaveText.h>
#endif

#include "../helpers/graphics.C"

const double kVerySmall = 1e-5;
TH1 *extractNEFSpectrum(THnSparse *jetSparse, double  ptmin, double ptmax){
  auto globfirst = jetSparse->GetAxis(0)->GetFirst(), globlast = jetSparse->GetAxis(0)->GetLast(),
       binmin = jetSparse->GetAxis(0)->FindBin(ptmin + kVerySmall), binmax = jetSparse->GetAxis(0)->FindBin(ptmax - kVerySmall);
  jetSparse->GetAxis(0)->SetRange(binmin, binmax);
  auto spectrum =  jetSparse->Projection(1);
  spectrum->SetDirectory(nullptr);
  spectrum->Scale(1./spectrum->Integral());
  jetSparse->GetAxis(0)->SetRange(globfirst, globlast);
  return spectrum;
}

THnSparse *ExtractJetTHnSparse(const char *filename, const char *trigger, double jetradius) {
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename, "READ"));
  reader->cd(Form("JetConstituentQA_%s", trigger));
  auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
  auto jetsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hJetCounterfulljets_R%02d", int(jetradius*10.))));
  return jetsparse;
}

void makeDataMCComparisonNEF(const char *datafile, const char *mcfile, const char *trigger, double jetradius) {
  auto jetsparseData = std::unique_ptr<THnSparse>(ExtractJetTHnSparse(datafile, trigger, jetradius)),
       jetsparseMC = std::unique_ptr<THnSparse>(ExtractJetTHnSparse(mcfile, trigger, jetradius));

  auto plot = new TCanvas("NEFplot", "NEF comparison", 1200, 1000);
  plot->Divide(3,3);

  Style datastyle{kBlack, 24}, mcstyle{kRed, 25};

  auto current = 1;
  for(auto iptmin : ROOT::TSeqI(20, 200, 20)){
    auto ptmin = double(iptmin), ptmax = double(iptmin) + 20.;
    bool doLegend = current == 1;
    plot->cd(current++);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.06);

    auto frame = new TH1F(Form("frame_%d_%d", int(ptmin), int(ptmax)), "; NEF; 1/N_{jet} dN_{jet}/dNEF", 100, 0., 1.);
    frame->SetDirectory(nullptr);
    frame->SetStats(false);
    frame->GetYaxis()->SetRangeUser(0., 0.1);
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

    auto dataspec = extractNEFSpectrum(jetsparseData.get(), ptmin, ptmax),
         mcspec = extractNEFSpectrum(jetsparseMC.get(), ptmin, ptmax);
    datastyle.SetStyle(*dataspec);
    mcstyle.SetStyle(*mcspec);
    dataspec->Draw("epsame");
    mcspec->Draw("epsame");
    if(doLegend) {
      leg->AddEntry(dataspec, "Data", "lep");
      leg->AddEntry(mcspec, "MC", "lep");
    }
  }
}