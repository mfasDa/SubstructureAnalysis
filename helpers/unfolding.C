#ifndef __UNFOLDING_C__
#define __UNFOLDING_C__

#ifndef __CLING__
#include <iostream>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <TH2.h>
#include <TMath.h>
#include "RooUnfoldResponse.h"
#endif

#include "root.C"

TH2 *Refold(const TH2 *histtemplate, const TH2 *unfolded, const RooUnfoldResponse &response) {
  // outer loop - reconstructed
  // inner loop - true
  auto refolded = static_cast<TH2 *>(histcopy(histtemplate));
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

TH1 *MakeRefolded1D(const TH1 *histtemplate, const TH1 *unfolded, const RooUnfoldResponse &response){
  auto refolded = histcopy(histtemplate);
  refolded->Sumw2();
  refolded->Reset();
  
  for(auto binptsmear : ROOT::TSeqI(0, refolded->GetXaxis()->GetNbins())) {
    double effect = 0, error = 0;
    for(auto binpttrue : ROOT::TSeqI(0, unfolded->GetXaxis()->GetNbins())) {
      effect += unfolded->GetBinContent(binpttrue+1) * response(binptsmear, binpttrue);
      error += TMath::Power(unfolded->GetBinError(binpttrue+1) * response(binptsmear, binpttrue), 2);
    }
    refolded->SetBinContent(binptsmear + 1, effect);
    refolded->SetBinError(binptsmear +1, TMath::Sqrt(error));
  }
  return refolded;
}

TH2 *sliceResponseObservableBase(TH2 *responsematrix, TH2 *truth, TH2 *smeared, const char *nameObservable, int binpttrue = -1, int binptsmear = -1) {
  const int nbinspttrue = truth->GetYaxis()->GetNbins(),
            nbinsptsmear = smeared->GetYaxis()->GetNbins(),
            nbinsobstrue = truth->GetXaxis()->GetNbins(),
            nbinsobssmear = smeared->GetXaxis()->GetNbins();

  if(binpttrue > nbinspttrue) return nullptr;
  if(binptsmear > nbinsptsmear) return nullptr;

  int binpttruemin = binpttrue < 0 ? 0 : binpttrue, binpttruemax = binpttrue < 0 ? nbinspttrue : binpttrue+1,
      binptsmearmin = binptsmear < 0 ? 0 : binptsmear, binptsmearmax = binptsmear < 0 ? nbinsptsmear : binptsmear+1;

  auto result = new TH2D(Form("responsematrix_slice%s_ptrue_%d_%d_ptsmear_%d_%d", nameObservable, binpttruemin, binpttruemax, binptsmearmin, binptsmearmax),
                         Form("Response matrix sliced in %s for %.1f GeV/c < p_{t,true} < %.1f GeV/c and %.1f GeV/c < p_{t,meas} < %.1f GeV/c", nameObservable, 
                                                    truth->GetYaxis()->GetBinLowEdge(binpttruemin+1), truth->GetYaxis()->GetBinLowEdge(binpttruemax+1), 
                                                    smeared->GetYaxis()->GetBinLowEdge(binptsmearmin+1), smeared->GetYaxis()->GetBinLowEdge(binptsmearmax+1)),
                         nbinsobssmear, smeared->GetXaxis()->GetXbins()->GetArray(), nbinsobstrue, smeared->GetXaxis()->GetXbins()->GetArray());
  TH2D work(*result);
  for(int mypttruebin : ROOT::TSeqI(binpttruemin, binpttruemax)){
    std::cout << "adding true pt bin " << mypttruebin << " (" << truth->GetYaxis()->GetBinLowEdge(mypttruebin+1) << " ... " << truth->GetYaxis()->GetBinUpEdge(mypttruebin+1) << ")" << std::endl;
    for(int mypttsmearbin : ROOT::TSeqI(binptsmearmin, binptsmearmax)){
      std::cout << "adding measured pt bin " << mypttsmearbin << " (" << smeared->GetYaxis()->GetBinLowEdge(mypttsmearbin+1) << " ... " << smeared->GetYaxis()->GetBinUpEdge(mypttsmearbin+1) << ")" << std::endl;
      work.Reset();
      for(auto binshapesmear : ROOT::TSeqI(0, nbinsobssmear)){
        int indexsmear = mypttsmearbin * nbinsobssmear + binshapesmear;
        for(auto binshapetrue : ROOT::TSeqI(0, nbinsobstrue)){
          int indextrue = mypttruebin * nbinsobstrue + binshapetrue;
          work.SetBinContent(binshapesmear+1, binshapetrue+1, responsematrix->GetBinContent(indexsmear+1, indextrue+1));
          work.SetBinError(binshapesmear+1, binshapetrue+1, responsematrix->GetBinError(indexsmear+1, indextrue+1));
        }
      }
      result->Add(&work);
    }
  }
  return result;
}

TH2 *sliceResponsePtBase(TH2 *responsematrix, TH2 *truth, TH2 *smeared, const char *nameObservable, int binobstrue = -1, int binobssmear = -1){
  const int nbinspttrue = truth->GetYaxis()->GetNbins(),
            nbinsptsmear = smeared->GetYaxis()->GetNbins(),
            nbinsobstrue = truth->GetXaxis()->GetNbins(),
            nbinsobssmear = smeared->GetXaxis()->GetNbins();

  if(binobstrue > nbinsobstrue) return nullptr;
  if(binobssmear > nbinsobssmear) return nullptr;

  int binobstruemin = binobstrue < 0 ? 0 : binobstrue, binobstruemax = binobstrue < 0 ? nbinsobstrue : binobstrue+1,
      binobssmearmin = binobssmear < 0 ? 0 : binobssmear, binobssmearmax = binobssmear < 0 ? nbinsobssmear : binobssmear+1;

  auto result = new TH2D(Form("responsematrix_slicept_%strue_%d_%d_%ssmear_%d_%d", nameObservable, binobstruemin, binobstruemax, nameObservable, binobssmearmin, binobssmearmax),
                         Form("Response matrix sliced in p_{t} for %.1f < %s_{true} < %.2f  and %.2f < %s_{meas} < %.2f", 
                                                    truth->GetXaxis()->GetBinLowEdge(binobstruemin+1), nameObservable, truth->GetXaxis()->GetBinLowEdge(binobstruemax+1),
                                                    smeared->GetXaxis()->GetBinLowEdge(binobssmearmin+1), nameObservable, smeared->GetXaxis()->GetBinLowEdge(binobssmearmax+1)),
                         nbinsptsmear, smeared->GetYaxis()->GetXbins()->GetArray(), nbinspttrue, truth->GetYaxis()->GetXbins()->GetArray());
  TH2D work(*result);
  for(auto binshapetrue : ROOT::TSeqI(binobstruemin, binobstruemax)){
    std::cout << "adding true shape bin " << binshapetrue << " (" << truth->GetXaxis()->GetBinLowEdge(binshapetrue+1) << " ... " << truth->GetXaxis()->GetBinUpEdge(binshapetrue+1) << ")" << std::endl;
    for(auto binshapesmear : ROOT::TSeqI(binobssmearmin, binobssmearmax)){
      std::cout << "adding measured shape bin " << binshapesmear << " (" << smeared->GetXaxis()->GetBinLowEdge(binshapesmear+1) << " ... " << smeared->GetXaxis()->GetBinUpEdge(binshapesmear+1) << ")" << std::endl;
      work.Reset();
      for(int mypttsmearbin : ROOT::TSeqI(0, nbinsptsmear)){
        int indexsmear = mypttsmearbin * nbinsobssmear + binshapesmear;
        for(int mypttruebin : ROOT::TSeqI(0, nbinspttrue)){
          int indextrue = mypttruebin * nbinsobstrue + binshapetrue;
          work.SetBinContent(mypttsmearbin+1, mypttruebin+1, responsematrix->GetBinContent(indexsmear+1, indextrue+1));
          work.SetBinError(mypttsmearbin+1, mypttruebin+1, responsematrix->GetBinError(indexsmear+1, indextrue+1));
        }
      }
      result->Add(&work);
    }
  }
  return result;
}

TH2 *sliceRepsonseObservable(RooUnfoldResponse &response, const char *nameObservable, int binpttrue = -1, int binptsmear = -1) {
  return sliceResponseObservableBase(response.Hresponse(), static_cast<TH2 *>(response.Htruth()), static_cast<TH2 *>(response.Hmeasured()), nameObservable,  binpttrue, binptsmear);
}

TH2 *sliceRepsonsePt(RooUnfoldResponse &response, const char *nameObservable, int binobstrue = -1, int binobssmear = -1) {
  return sliceResponsePtBase(response.Hresponse(), static_cast<TH2 *>(response.Htruth()), static_cast<TH2 *>(response.Hmeasured()), nameObservable, binobstrue, binobssmear);
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

TH2D *CorrelationHistPt(const TMatrixD &cov, const char *name, const char *title,
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

TH2D *CorrelationHistShape(const TMatrixD &cov, const char *name, const char *title,
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

TH2 *makeNormalizedResponse(TH2 *responsein) {
    auto normalised = static_cast<TH2 *>(histcopy(responsein));
    std::unique_ptr<TH1> projected(responsein->ProjectionY("_py", 1, responsein->GetNbinsX()));
    for(auto ptpart : ROOT::TSeqI(0, responsein->GetNbinsY())) {
        auto weight = projected->GetBinContent(ptpart+1);
        if(weight){
            for(auto ptdet : ROOT::TSeqI(0, responsein->GetNbinsX())){
                auto bc = responsein->GetBinContent(ptdet+1, ptpart+1);
                normalised->SetBinContent(ptdet+1, ptpart+1, bc/weight);
            }
        }
    }
    return normalised;
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

TH2 *TruncateResponse(TH2 *inputhist, TH2 *responsetemplate, const char *name , const char *title){
  auto truncated = static_cast<TH2 *>(histcopy(responsetemplate));
  truncated->SetNameTitle(name, title);
  for(auto binx : ROOT::TSeqI(0, truncated->GetXaxis()->GetNbins())){
    int binxNonTrunc = inputhist->GetXaxis()->FindBin(truncated->GetXaxis()->GetBinCenter(binx+1));
    for(auto biny : ROOT::TSeqI(0, truncated->GetYaxis()->GetNbins())){
      truncated->SetBinContent(binx+1, biny+1, inputhist->GetBinContent(binxNonTrunc, biny+1));
    }
  }
  return truncated;
}
#endif