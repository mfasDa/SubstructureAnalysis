#include "../../meta/stl.C"
#include "../../meta/root.C"

std::vector<double> getJetPtBinningNonLinSmearLargeOption1(){
  // default binning:  {20., 22., 24., 26., 28., 30., 32., 34., 36., 38., 40., 45., 50., 
  //                    55., 60., 70., 80., 90., 100., 110., 120., 140., 160., 180. 200.};
  return {20., 21.5, 23., 24.5, 26., 27.5, 29., 30.5, 33., 34.5, 36., 40., 44., 
          48., 52., 60., 68., 76., 84., 92., 104., 119., 144., 172., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeOption2(){
  // default binning:  {20., 22., 24., 26., 28., 30., 32., 34., 36., 38., 40., 45., 50., 
  //                    55., 60., 70., 80., 90., 100., 110., 120., 140., 160., 180. 200.};
  return {20., 22.5, 25., 27.5, 30., 32.5, 35., 37.5, 40., 42.5, 45., 52., 59., 
          56., 63., 74., 85., 96., 107., 118., 132., 149., 166., 183., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeOption3(){
  // default binning:  {20., 22., 24., 26., 28., 30., 32., 34., 36., 38., 40., 45., 50., 
  //                    55., 60., 70., 80., 90., 100., 110., 120., 140., 160., 180., 200.};
 return {20., 23., 25., 27., 29., 31., 33., 35., 37., 39., 41., 46., 51., 56., 61., 71., 81., 
         91., 101., 111., 121., 141., 161., 181., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeOption4(){
  // default binning:  {20., 22., 24., 26., 28., 30., 32., 34., 36., 38., 40., 45., 50., 
  //                    55., 60., 70., 80., 90., 100., 110., 120., 140., 160., 180., 200.};
  return {20., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 44., 49., 54., 59., 69., 79., 89., 
          99., 109., 119., 139., 159., 179., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLarge(const std::string_view option){
    std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
      {"option1", getJetPtBinningNonLinSmearLargeOption1},  
      {"option2", getJetPtBinningNonLinSmearLargeOption2},
      {"option3", getJetPtBinningNonLinSmearLargeOption3},
      {"option4", getJetPtBinningNonLinSmearLargeOption4}
    };
    auto functor = functors.find(std::string(option));
    if(functor == functors.end()) return {};
    return functor->second();
}

std::vector<double> getJetPtBinningNonLinTrueLarge(){
  std::vector<double> result;
  result.emplace_back(0.);
  result.emplace_back(20.);
  double current = 20.;
  while(current < 80.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 200){
    current += 20;
    result.push_back(current);
  }
  while(current < 280.){
    current += 40.;
    result.push_back(current);
  }
  result.emplace_back(600.);
  return result;
}
