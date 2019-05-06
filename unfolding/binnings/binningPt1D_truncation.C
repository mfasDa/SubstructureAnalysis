#include "binninghelper.cpp"

std::vector<double> getJetPtBinningNonLinSmearLoose(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 5);
  binning.AddStep(120., 10.);
  binning.AddStep(140., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearStrong(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 5);
  binning.AddStep(120., 10.);
  binning.AddStep(140., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeLoose(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2);
  binning.AddStep(60., 5.);
  binning.AddStep(120., 10.);
  binning.AddStep(200., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeStrong(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2);
  binning.AddStep(60., 5.);
  binning.AddStep(120., 10.);
  binning.AddStep(200., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearFineLoose(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 4.);
  binning.AddStep(120., 5.);
  binning.AddStep(200, 10.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearFineStrong(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 4.);
  binning.AddStep(120., 5.);
  binning.AddStep(200, 10.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300Loose(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 4.);
  binning.AddStep(120., 5.);
  binning.AddStep(200., 10.);
  binning.AddStep(300., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300Strong(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 4.);
  binning.AddStep(120., 5.);
  binning.AddStep(200., 10.);
  binning.AddStep(300., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240Loose(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 4.);
  binning.AddStep(120., 5.);
  binning.AddStep(200., 10.);
  binning.AddStep(240., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240Strong(){
  binninghelper binning;
  binning.SetMinimum(10.);
  binning.AddStep(40., 2.);
  binning.AddStep(60., 4.);
  binning.AddStep(120., 5.);
  binning.AddStep(200., 10.);
  binning.AddStep(240., 20.);
  return binning.CreateCombinedBinning();
}

std::vector<double>getJetPtBinningNonLinSmear(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"loose", getJetPtBinningNonLinSmearLoose},
    {"strong", getJetPtBinningNonLinSmearStrong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
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

std::vector<double>getJetPtBinningNonLinSmearFine(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"loose", getJetPtBinningNonLinSmearFineLoose},
    {"strong", getJetPtBinningNonLinSmearFineStrong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}

std::vector<double>getJetPtBinningNonLinSmearUltra300(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"loose", getJetPtBinningNonLinSmearUltra300Loose},
    {"strong", getJetPtBinningNonLinSmearUltra300Strong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}

std::vector<double>getJetPtBinningNonLinSmearUltra300(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"loose", getJetPtBinningNonLinSmearUltra300Loose},
    {"strong", getJetPtBinningNonLinSmearUltra300Strong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}