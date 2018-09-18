#ifndef __CLING__
#include <vector>
#endif

std::vector<double> getZgBinningFine(){
  std::vector<double> result;
  result.push_back(0.);
  for(double d = 0.1; d <= 0.5; d += 0.05) result.push_back(d);
  return result;
}

std::vector<double> getZgBinningFineFake(){
  std::vector<double> result;
  for(double d = 0.1; d <= 0.5; d += 0.05) result.push_back(d);
  result.push_back(0.6);
  return result;
}

std::vector<double> getZgBinningCoarse(){
  std::vector<double> result;
  result.push_back(0.);
  for(double d = 0.1; d <= 0.5; d += 0.1) result.push_back(d);
  return result;
}

std::vector<double> getMinBiasPtBinningRealisticOld() {
  std::vector<double> result;
  double current = 20;
  result.push_back(current);
  while(current < 30.) {
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

std::vector<double> getMinBiasPtBinningRealistic() {
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

std::vector<double> getEJ1PtBinningRealistic(){
  std::vector<double> result;
  double current = 80.;
  result.push_back(current);
  while(current < 120.){
    current += 5.;
    result.push_back(current);
  }
  while(current < 160.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 200.){
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ1PtBinningPart(){
  std::vector<double> result;
  result.push_back(0.);
  double current = 80.;
  result.push_back(current);
  while(current < 140.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 200.){
    current += 20.;
    result.push_back(current);
  }
  result.emplace_back(240.);
  result.emplace_back(400.);
  return result;
}

std::vector<double> getEJ2PtBinningRealistic(){
  std::vector<double> result;
  double current = 70.;
  result.push_back(current);
  while(current < 100.){
    current += 5.;
    result.push_back(current);
  }
  while(current < 120.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 140.) {
    current += 20.;
    result.push_back(current);
  }
  return result;
}

std::vector<double> getEJ2PtBinningPart(){
  std::vector<double> result;
  result.push_back(0);
  double current = 70.;
  result.push_back(current);
  while(current < 100.){
    current += 10.;
    result.push_back(current);
  }
  while(current < 140.) {
    current += 20.;
    result.push_back(current);
  }
  result.push_back(400.);
  return result;
}

std::vector<double> getPtBinningRealistic(const std::string_view trigger) {
  if(trigger == "INT7") {
    //return getMinBiasPtBinningRealistic();
    return getMinBiasPtBinningRealistic();
  } else if(trigger == "EJ2") {
    return getEJ2PtBinningRealistic();
  } else if(trigger == "EJ1") {
    return getEJ1PtBinningRealistic();
  }
}

std::vector<double> getPtBinningPart(const std::string_view trigger) {
  if(trigger == "INT7") {
    //return getMinBiasPtBinningPart();
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

