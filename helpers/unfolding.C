#ifndef __UNFOLDING_C__
#define __UNFOLDING_C__

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
      nbinsptsmear = refolded->GetYaxis()->GetNbins(), nbinspttrue = unfolded->GetYaxis()->GetNbins();

  for (auto binshapesmear : ROOT::TSeqI(0, nbinsprobesmear)) {      // bins shape, reconstructed
    for (auto binptsmear : ROOT::TSeqI(0, nbinsptsmear)) {       // bins pt, reconstructed
      int indexmeas = binshapesmear + nbinsprobesmear * binptsmear;
      double effects = 0;
      double error = 0;
      for (auto binshapetrue : ROOT::TSeqI(0, nbinsprobetrue)) {   // bins shape, true
        for (auto binpttrue : ROOT::TSeqI(0, nbinspttrue)) {    // bins pt, true
          int indextrue = binshapetrue + nbinsprobetrue * binpttrue;

          //std::cerr << "indexm " << indexm << ", indext " << indext <<std::endl;
          effects += unfolded->GetBinContent(binshapetrue + 1, binpttrue + 1) * response(indexmeas, indextrue);
          error += TMath::Power(unfolded->GetBinError(binshapetrue + 1, binpttrue + 1) * response(indexmeas, indextrue), 2);
        }
      }
      refolded->SetBinContent(binshapesmear + 1, binptsmear + 1, effects);
      refolded->SetBinError(binshapesmear + 1, binptsmear + 1, TMath::Sqrt(error));
    }
  }
  return refolded;
}

TH2 *ProjectResponseObservable(RooUnfoldResponse &response, const char *name, const char *title, int binpttrue){
  auto resptrue = response.Htruth(), respmeasured = response.Hmeasured();
  auto matrix = response.Hresponse();
  std::vector<double> binstrue = {resptrue->GetXaxis()->GetBinLowEdge(1)}, binsmeasured = {respmeasured->GetXaxis()->GetBinLowEdge(1)};
  for(auto b : ROOT::TSeqI(0, resptrue->GetXaxis()->GetNbins())) {
    binstrue.push_back(resptrue->GetXaxis()->GetBinUpEdge(b+1));
  }
  for(auto b : ROOT::TSeqI(0, respmeasured->GetXaxis()->GetNbins())) {
    binsmeasured.push_back(respmeasured->GetXaxis()->GetBinUpEdge(b+1));
  }
  TH2D * hist = new TH2D(name, title, binstrue.size()-1, binstrue.data(), binsmeasured.size()-1, binsmeasured.data());
  hist->Sumw2();

  for(auto binptrec : ROOT::TSeqI(0, respmeasured->GetYaxis()->GetNbins())){      // bins pt det
    TH2D myptrecbin(*hist);
    myptrecbin.Reset();
    for(auto iobstrue : ROOT::TSeqI(0, resptrue->GetNbinsX())) {
      for(auto iobsmeas : ROOT::TSeqI(0, respmeasured->GetNbinsX())) {
        int ibinmeas = iobsmeas + (binptrec * binsmeasured.size()-1),
            ibintrue = iobstrue + (binpttrue * binstrue.size() -1);
        myptrecbin.SetBinContent(iobstrue+1, iobsmeas+1, matrix->GetBinContent(matrix->GetXaxis()->FindBin(ibinmeas + 0.5), matrix->GetYaxis()->FindBin(ibintrue + 0.5)));
        myptrecbin.SetBinError(iobstrue+1, iobsmeas+1, matrix->GetBinContent(matrix->GetXaxis()->FindBin(ibinmeas + 0.5), matrix->GetYaxis()->FindBin(ibintrue + 0.5)));
      }
    }
    hist->Add(&myptrecbin);
  }
  return hist;
}

void CheckNormalized(const RooUnfoldResponse &response, int nbinsprobetrue, int nbinsprobesmear, int nbinspttrue, int nbinsptsmear){
  // outer loop - true
  // inner loop - reconstructed
  for (auto binshapetrue : ROOT::TSeqI(0,nbinsprobetrue)) {        // bins shape, true 
    for (auto binpttrue : ROOT::TSeqI(0,nbinspttrue)) {         // bins pt, true
      double effects = 0;
      for (auto binshapesmeared : ROOT::TSeqI(0,nbinsprobesmear)) {   // bins angularity, reconstructed
        for (auto binptsmeared : ROOT::TSeqI(0, nbinsptsmear)) // bins pt, reconstructed
        {
          int indexm = binshapesmeared + nbinsprobesmear * binptsmeared;
          int indext = binshapetrue + nbinsprobetrue * binpttrue;
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
#endif