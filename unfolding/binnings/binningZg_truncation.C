#ifndef __CLING__
#include <vector>
#include <RStringView.h>
#endif

std::vector<double> getZgBinningCoarse(){
  std::vector<double> result;
  result.push_back(0.);
  for(double d = 0.1; d <= 0.5; d += 0.1) result.push_back(d);
  return result;
}

std::vector<double> getMinBiasPtBinningDetLoose() {
  std::vector<double> result;
  double current = 10;
  result.push_back(current);
  while(current < 40.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 60.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 80.) {
    current += 20.;
    result.push_back(current);
  }
  while(current < 120.) {
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getMinBiasPtBinningDetStrong() {
  std::vector<double> result;
  double current = 25;
  result.push_back(current);
  while(current < 40.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 60.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 80.) {
    current += 20.;
    result.push_back(current);
  }
  while(current < 120.) {
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getMinBiasPtBinningPart() {
  std::vector<double> result;
  double current = 0;
  result.push_back(current);
  while(current < 20.) {
    current += 20.;
    result.push_back(current);
  }
  while(current < 40.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 80.) {
    current += 20.;
    result.push_back(current);
  }
  result.push_back(120.);
  result.push_back(240.);
  return result;
}

std::vector<double> getEJ1PtBinningDetLoose(){
  std::vector<double> result;
  double current = 75.;
  result.push_back(current);
  while(current < 100.){
    current += 5.;
    result.push_back(current);
  }
  while(current < 120.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 160.){
    current += 20.;
    result.push_back(current);
  }
  while(current < 200.){
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ1PtBinningDetStrong(){
  std::vector<double> result;
  double current = 85.;
  result.push_back(current);
  while(current < 100.){
    current += 5.;
    result.push_back(current);
  }
  while(current < 120.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 160.){
    current += 20.;
    result.push_back(current);
  }
  while(current < 200.){
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ1PtBinningPart(){
  std::vector<double> result;
  double current = 0.;
  result.push_back(current);
  while(current < 80) {
    current += 80.;
    result.push_back(current);
  }
  while(current < 160.){
    current += 20.;
    result.push_back(current);
  }
  while(current < 200.){
    current += 40.;
    result.push_back(current);
  }
  result.emplace_back(400.);
  return result;
}

std::vector<double> getEJ2PtBinningDetLoose(){
  std::vector<double> result;
  double current = 55.;
  result.push_back(current);
  while(current < 70.){
    current += 5.;
    result.push_back(current);
  }
  while(current < 100.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 120.) {
    current += 20.;
    result.push_back(current);
  }
  while(current < 160.) {
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ2PtBinningDetStrong(){
  std::vector<double> result;
  double current = 65.;
  result.push_back(current);
  while(current < 70.){
    current += 5.;
    result.push_back(current);
  }
  while(current < 100.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 120.) {
    current += 20.;
    result.push_back(current);
  }
  while(current < 160.) {
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ2PtBinningPart(){
  std::vector<double> result;
  double current = 0.;
  result.push_back(current);
  while(current < 60.){
    current += 60.;
    result.push_back(current);
  }
  while(current < 120.){
    current += 20.;
    result.push_back(current);
  }
  while(current < 160.) {
    current += 40.;
    result.push_back(current);
  }
  while(current < 280.) {
    current += 120.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getPtBinningRealistic(const std::string_view trigger, bool loose) {
  if(trigger == "INT7") {
    return loose ? getMinBiasPtBinningDetLoose() : getMinBiasPtBinningDetStrong();
  } else if(trigger == "EJ2") {
    return loose ? getEJ2PtBinningDetLoose() : getEJ2PtBinningDetStrong();
  } else if(trigger == "EJ1") {
    return loose ? getEJ1PtBinningDetLoose() : getEJ1PtBinningDetStrong();
  }
}

std::vector<double> getPtBinningPart(const std::string_view trigger) {
  if(trigger == "INT7") {
    return getMinBiasPtBinningPart();
  } else if(trigger == "EJ2") {
    return getEJ2PtBinningPart();
  } else if(trigger == "EJ1") {
    return getEJ1PtBinningPart();
  }
}
