#include "../../meta/stl.C"
#include "../../meta/root.C"

std::vector<double> getJetPtBinningNonLinSmearLargeLoose(){
  std::vector<double> result;
  result.emplace_back(14.);
  double current = 14.;
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

std::vector<double> getJetPtBinningNonLinSmearLargeStrong(){
  std::vector<double> result;
  result.emplace_back(24.);
  double current = 24.;
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

std::vector<double>getJetPtBinningNonLinSmearLarge(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"loose", getJetPtBinningNonLinSmearLargeLoose},
    {"strong", getJetPtBinningNonLinSmearLargeStrong}
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