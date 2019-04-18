#include "../../meta/stl.C"

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

std::vector<double> getJetPtBinningNonLinSmearLargeLow(){
  std::vector<double> result;
  result.emplace_back(10.);
  double current = 10.;
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

std::vector<double> getJetPtBinningNonLinSmearLargeFineLow(){
  std::vector<double> result;
  result.emplace_back(10.);
  double current = 10.;
  while(current < 40.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 60){
    current += 4.;
    result.push_back(current);
  }
  while(current < 120){
    current += 5.;
    result.push_back(current);
  }
  while(current < 200){
    current += 10.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningNonLinSmearUltra300(){
  std::vector<double> result;
  result.emplace_back(10.);
  double current = 10.;
  while(current < 40.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 60){
    current += 4.;
    result.push_back(current);
  }
  while(current < 120){
    current += 5.;
    result.push_back(current);
  }
  while(current < 200){
    current += 10.;
    result.push_back(current);
  }
  while(current < 300){
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningNonLinSmearUltra240(){
  std::vector<double> result;
  result.emplace_back(10.);
  double current = 10.;
  while(current < 40.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 60){
    current += 4.;
    result.push_back(current);
  }
  while(current < 120){
    current += 5.;
    result.push_back(current);
  }
  while(current < 200){
    current += 10.;
    result.push_back(current);
  }
  while(current < 240){
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningLinSmearLarge(){
  std::vector<double> result;
  result.emplace_back(20.);
  double current = 20.;
  while(current < 200.) {
    current += 5.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningNonLinTrueLargeLow(){
  std::vector<double> result;
  result.emplace_back(5.);
  result.emplace_back(10.);
  double current = 10.;
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
  result.emplace_back(500.);
  return result;
}

std::vector<double> getJetPtBinningNonLinTrueLargeFineLow(){
  std::vector<double> result;
  result.emplace_back(5.);
  result.emplace_back(10.);
  double current = 10.;
  while(current < 60.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 140.) {
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
  result.emplace_back(500.);
  return result;
}

std::vector<double> getJetPtBinningNonLinTrueUltra300(){
  std::vector<double> result;
  result.emplace_back(5.);
  result.emplace_back(10.);
  double current = 10.;
  while(current < 60.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 140.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 200){
    current += 20;
    result.push_back(current);
  }
  while(current < 520){
    current += 40;
    result.push_back(current);
  }
  result.emplace_back(600.);
  return result;
}

std::vector<double> getJetPtBinningNonLinTrueUltra240(){
  std::vector<double> result;
  result.emplace_back(5.);
  result.emplace_back(10.);
  double current = 10.;
  while(current < 60.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 140.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 200){
    current += 20;
    result.push_back(current);
  }
  while(current < 320){
    current += 40;
    result.push_back(current);
  }
  result.emplace_back(500.);
  return result;
}
