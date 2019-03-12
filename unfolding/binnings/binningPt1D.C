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

std::vector<double> getJetPtBinningNonLinSmearLargeFine(){
  std::vector<double> result;
  result.emplace_back(20.);
  double current = 20.;
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

std::vector<double> getJetPtBinningElianeSmear(){
  return {7., 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 125, 130.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeEJ1(){
  std::vector<double> result;
  result.emplace_back(30.);
  double current = 30.;
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
  result.emplace_back(5.);
  result.emplace_back(20.);
  double current = 20.;
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

std::vector<double> getJetPtBinningLinTrueLarge(){
  std::vector<double> result;
  result.emplace_back(0.);
  double current = 0.;
  while(current < 400.){
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getJetPtBinningElianeTrue(){
  return {5., 10, 20, 30, 40, 50, 60, 70, 80, 100, 120, 140, 190, 250};
}

std::vector<double> getJetPtBinningNonLinTrueLarge(){
  std::vector<double> result;
  result.emplace_back(5.);
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
  result.emplace_back(500.);
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

std::vector<double> getJetPtBinningNonLinTrueLargeFine(){
  std::vector<double> result;
  result.emplace_back(5.);
  result.emplace_back(20.);
  double current = 20.;
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

std::vector<double> getJetPtBinningNonLinTrueLargeEJ1(){
  std::vector<double> result;
  result.emplace_back(0.);
  result.emplace_back(30.);
  double current = 30.;
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