#ifndef __CLING__
#include <ROOT/TSeq.hxx>
#include <TH1.h>
#include <TMath.h>
#endif

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

void invert(TH1 *hist){
  for(auto b : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())){
    auto val = hist->GetBinContent(b+1), err = hist->GetBinError(b+1);
    if(val){
      hist->SetBinContent(b+1, 1./val);
      hist->SetBinError(b+1, err/(val*val));
    }
  }
}