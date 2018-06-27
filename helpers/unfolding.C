#ifndef __CLING__
#include <vector>
#include <ROOT/TSeq.hxx>
#include <TH2.h>
#include <TMath.h>
#include "RooUnfoldResponse.h"
#endif

TH2 *Refold(const TH2 *histtemplate, const TH2 *unfolded, const RooUnfoldResponse &response) {
  // outer loop - reconstructed
  // inner loop - true
  auto refolded = static_cast<TH2 *>(histtemplate->Clone());
  refolded->Sumw2();

  int nbinsprobesmear = refolded->GetXaxis()->GetNbins(), nbinsprobetrue = unfolded->GetXaxis()->GetNbins(),
      nbinsptsmear = refolded->GetYaxis()->GetNbins(), nbinspttrue = unfolded->GetXaxis()->GetNbins();

  for (auto i : ROOT::TSeqI(0, nbinsprobesmear)) {      // bins shape, reconstructed
    for (auto j : ROOT::TSeqI(0, nbinsptsmear)) {       // bins pt, reconstructed
      int indexmeas = i + nbinsprobesmear * j;
      double effects = 0;
      double error = 0;
      for (auto k : ROOT::TSeqI(0, nbinsprobetrue)) {   // bins shape, true
        for (auto l : ROOT::TSeqI(0, nbinspttrue)) {    // bins pt, true
          int indextrue = k + nbinsprobetrue * l;

          //std::cerr << "indexm " << indexm << ", indext " << indext <<std::endl;
          effects += unfolded->GetBinContent(k + 1, l + 1) * response(indexmeas, indextrue);
          error += TMath::Power(unfolded->GetBinError(k + 1, l + 1) * response(indexmeas, indextrue), 2);
        }
      }
      refolded->SetBinContent(i + 1, j + 1, effects);
      refolded->SetBinError(i + 1, j + 1, TMath::Sqrt(error));
    }
  }
  return refolded;
}

void CheckNormalized(const RooUnfoldResponse &response, int nbinsprobetrue, int nbinsprobesmear, int nbinspttrue, int nbinsptsmear){
  // outer loop - true
  // inner loop - reconstructed
  for (auto k : ROOT::TSeqI(0,nbinsprobetrue)) {        // bins shape, true 
    for (auto l : ROOT::TSeqI(0,nbinspttrue)) {         // bins pt, true
      double effects = 0;
      for (auto i : ROOT::TSeqI(0,nbinsprobesmear)) {   // bins angularity, reconstructed
        for (auto j : ROOT::TSeqI(0, nbinsptsmear)) // bins pt, reconstructed
        {
          int indexm = i + nbinsprobesmear * j;
          int indext = k + nbinsprobetrue * l;
          effects += response(indexm, indext);
        }
      }
    }
  }
}

TH2D *CorrelationHistShape(const TMatrixD &cov, const char *name, const char *title,
                           Int_t nBinsShape, Int_t nBinsPt, Int_t kBinShapeTrue) { 
  auto pearson = new TH2D(name, title, nBinsPt, 0, nBinsPt, nBinsPt, 0, nBinsPt);
  for (auto x : ROOT::TSeqI(0, nBinsPt)) {
    for (auto y : ROOT::TSeqI(0, nBinsPt)) {
      auto indexx = kBinShapeTrue + nBinsShape * x;   // pt, true
      auto indexy = kBinShapeTrue + nBinsShape * y;   // pt, smeared
      auto Vv = cov(indexx, indexx) * cov(indexy, indexy);
      if (Vv > 0.0) pearson->SetBinContent(x + 1, y + 1, cov(indexx, indexy) / std::sqrt(Vv));
    }
  }
  return pearson;
}

TH2D *CorrelationHistPt(const TMatrixD &cov, const char *name, const char *title,
                        Int_t nbinsShapeTrue, Int_t nbinsPtTrue, Int_t kBinPtTrue) {
  auto pearson = new TH2D(name, title, nbinsShapeTrue, 0, nbinsShapeTrue, nbinsShapeTrue, 0, nbinsShapeTrue);
  for (auto x : ROOT::TSeqI(0, nbinsShapeTrue)) {
    for (auto y : ROOT::TSeqI(0, nbinsShapeTrue)) {
      auto indexx = x + nbinsShapeTrue * kBinPtTrue;    // shape, true
      auto indexy = y + nbinsShapeTrue * kBinPtTrue;    // shape, smeared
      auto Vv = cov(indexx, indexx) * cov(indexy, indexy);
      if (Vv > 0.0) pearson->SetBinContent(x + 1, y + 1, cov(indexx, indexy) / std::sqrt(Vv));
    }
  }
  return pearson;
}

void Normalize2D(TH2 *h) {
  std::vector<double> norm(h->GetYaxis()->GetNbins());
  for(auto biny : ROOT::TSeqI(0, h->GetYaxis()->GetNbins())) {
    norm[biny] = 0;
    for(auto binx : ROOT::TSeqI(0, h->GetNbinsX())) {
      norm[biny] += h->GetBinContent(binx+1, biny+1);
    }
  }

  for(auto biny : ROOT::TSeqI(0, h->GetYaxis()->GetNbins())) {
    for(auto binx : ROOT::TSeqI(0, h->GetXaxis()->GetNbins())) {
      if (norm[biny] == 0) continue; 
      else {
        h->SetBinContent(binx+1, biny+1, h->GetBinContent(binx+1, biny+1)/norm[biny]);
        h->SetBinError(binx+1, biny+1, h->GetBinError(binx+1, biny+1)/norm[biny]);
      }
    }
  }
}

TH1D *TruncateHisto(TH1D *gr, Int_t nbinsold, Int_t lowold, Int_t highold, Int_t nbinsnew, Int_t lownew, Int_t highnew, Int_t lim) {
  TH1D *hTruncate = new TH1D("hTruncate", "", nbinsnew, lownew, highnew);
  hTruncate->Sumw2();
  for(auto binx : ROOT::TSeqI(0, hTruncate->GetXaxis()->GetNbins())){
    hTruncate->SetBinContent(binx+1, gr->GetBinContent(lim + binx+1));
    hTruncate->SetBinError(binx+1, gr->GetBinError(lim + binx+1));
  }
  return hTruncate;
}