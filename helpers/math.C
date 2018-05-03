#ifndef __CLING__
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