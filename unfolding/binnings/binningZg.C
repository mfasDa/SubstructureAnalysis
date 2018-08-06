#ifndef __CLING__
#include <vector>
#endif

std::vector<double> getZgBinningFine(){
  std::vector<double> result;
  for(double d = 0.; d <= 0.5; d += 0.05) result.push_back(d);
  return result;
}

std::vector<double> getZgBinningCoarse(){
  std::vector<double> result;
  for(double d = 0.; d <= 0.5; d += 0.1) result.push_back(d);
  return result;
}

std::vector<double> getMinBiasPtBinningRealistic(bool cut = false) {
  std::vector<double> result;
  double current = 20;
  result.push_back(current);
  while(current < 40.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 60.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < (cut ? 80. : 120.)) {
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getMinBiasPtBinningPart() {
  std::vector<double> result;
  double current = 0;
  result.push_back(current);
  while(current < 20.) {
    current += 2.;
    result.push_back(current);
  }
  while(current < 40.) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 60.) {
    current += 10.;
    result.push_back(current);
  }
  while(current < 200.) {
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ1PtBinningRealistic(bool cut = false){
  std::vector<double> result;
  double current = 80.;
  result.push_back(current);
  while(current < 160.){
    current += 20.;
    result.push_back(current);
  }
  while(current < (cut ? 200. : 240.)){
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ1PtBinningPart(){
  std::vector<double> result;
  double current = 0.;
  result.push_back(current);
  while(current < 60) {
    current += 5.;
    result.push_back(current);
  }
  while(current < 160.){
    current += 20.;
    result.push_back(current);
  }
  while(current < 400.){
    current += 40.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ2PtBinningRealistic(bool cut = false){
  std::vector<double> result;
  double current = 60.;
  result.push_back(current);
  while(current < 100.){
    current += 10.;
    result.push_back(current);
  }
  while(current < (cut ? 120. : 140.)) {
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ2PtBinningPart(){
  std::vector<double> result;
  double current = 0.;
  result.push_back(current);
  while(current < 40.){
    current += 5.;
    result.push_back(current);
  }
  while(current < 100.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 200.) {
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getPtBinningRealistic(const std::string_view trigger, bool cut = false) {
  if(trigger == "INT7") {
    return getMinBiasPtBinningRealistic(cut);
  } else if(trigger == "EJ2") {
    return getEJ2PtBinningRealistic(cut);
  } else if(trigger == "EJ1") {
    return getEJ1PtBinningRealistic(cut);
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

std::vector<double> getPtBinningFine(const std::string_view trigger) {
  double ptmin, ptmax;
  if(trigger == "INT7") {
    ptmin = 20.; ptmax = 120.; 
  } else if(trigger == "EJ2") {
    ptmin = 60.; ptmax = 200.; 
  } else if(trigger == "EJ1") {
    ptmin = 80.; ptmax = 200.; 
  }
  std::vector<double> result;
  for(auto mypt = ptmin; mypt <= ptmax; mypt+=5) result.push_back(mypt);
  return result;
}

