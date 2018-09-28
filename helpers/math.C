#ifndef __MATH_C__
#define __MATH_C__

#ifndef __CLING__
#include <vector>
#include <ROOT/TSeq.hxx>
#include <TH1.h>
#include <TMath.h>
#endif

#include "root.C"

int getDigits(int number) {
  int ndigits(0);
  while(true){
    if(number / int(TMath::Power(10, ndigits))) ndigits++;
    else break;
  }
  return ndigits;
}

void normalizeBinWidth(TH1 *hist) {
  for(auto b : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())){
    auto bw = hist->GetXaxis()->GetBinWidth(b+1);
    hist->SetBinContent(b+1, hist->GetBinContent(b+1)/bw);
    hist->SetBinError(b+1, hist->GetBinError(b+1)/bw);
  }
}

std::vector<double> makeLinearBinning(int nbins, double ptmin, double ptmax) {
  std::vector<double> result;
  double bw = (ptmax-ptmin)/static_cast<double>(nbins);
  for(auto step = ptmin; step < ptmax; step += bw) result.push_back(step);
  return result;
}

void invert(TH1 *hist){
  for(auto b : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())){
    auto val = hist->GetBinContent(b+1), err = hist->GetBinError(b+1);
    if(val){
      hist->SetBinContent(b+1, 1./val);
      hist->SetBinError(b+1, err/(val*val));
    }
  }
}

TH1 *rebinPtSpectrum(TH1 *hinput, std::vector<double> xBins){
  std::unique_ptr<TH1> htmp(histcopy(hinput));
  htmp->Sumw2();
  for(auto ib : ROOT::TSeqI(0, htmp->GetNbinsX())){
    auto value = htmp->GetBinContent(ib+1);
    auto width = htmp->GetBinWidth(ib+1);
    auto center = htmp->GetBinCenter(ib+1);
    auto error = htmp->GetBinError(ib+1);
    htmp->SetBinContent(ib+1,value*width);
    htmp->SetBinError(ib+1,error*width);
  }
  auto hrebinned = htmp->Rebin(xBins.size()-1, Form("%sRebinned", hinput->GetName()), xBins.data());   
  for(auto ib : ROOT::TSeqI(0, hrebinned->GetNbinsX())){
    auto value = hrebinned->GetBinContent(ib+1);
    auto width = hrebinned->GetBinWidth(ib+1);
    auto center = hrebinned->GetBinCenter(ib+1);
    auto error = hrebinned->GetBinError(ib+1);
    hrebinned->SetBinContent(ib+1,value/width);
    hrebinned->SetBinError(ib+1,error/width);
  }
  return hrebinned;
}
#endif