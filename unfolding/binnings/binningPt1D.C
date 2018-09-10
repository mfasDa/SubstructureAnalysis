#ifndef __CLING__
#include <vector>
#endif

std::vector<double> getJetPtBinningNonLinSmear(){
  std::vector<double> result;
  result.emplace_back(20.);
  double current = 20.;
  while(current < 40.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 60){
    current += 5.;
    result.push_back(current);
  }
  while(current < 120){
    current += 10.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningNonLinSmearLarge(){
  std::vector<double> result;
  result.emplace_back(20.);
  double current = 20.;
  while(current < 40.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 60){
    current += 5.;
    result.push_back(current);
  }
  while(current < 120){
    current += 10.;
    result.push_back(current);
  }
  while(current < 200){
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningNonLinTrue(){
  std::vector<double> result;
  result.emplace_back(0.);
  double current = 0.;
  while(current < 20.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 80.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 160){
    current += 20;
    result.push_back(current);
  }
  while(current < 240.){
    current += 40.;
    result.push_back(current);
  }
  return result;
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
