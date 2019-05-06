#include "binninghelper.cpp"

std::vector<double> getJetPtBinningNonLinSmearLowerLoose(){
  binninghelper binning(8., {{40., 2.}, {60., 5.}, {120., 10.}, {140., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLowerStrong(){
  binninghelper binning(12., {{40., 2.}, {60., 5.}, {120., 10.}, {140., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUpperLoose(){
  binninghelper binning(10., {{40., 2.}, {60., 5.}, {120., 10.}, {145., 45.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUpperStrong(){
  binninghelper binning(10., {{40., 2.}, {60., 5.}, {120., 10.}, {135., 15.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeLowerLoose(){
  binninghelper binning(8., {{40., 2.}, {60., 5.}, {120., 10.}, {200., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeLowerStrong(){
  binninghelper binning(12., {{40., 2.}, {60., 5.}, {120., 10.}, {200., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeUpperLoose(){
  binninghelper binning(10., {{40., 2.}, {60., 5.}, {120., 10.}, {180., 20.}, {205., 55.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeUpperStrong(){
  binninghelper binning(10., {{40., 2.}, {60., 5.}, {120., 10.}, {180., 20.}, {195., 15.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearFineLowerLoose(){
  binninghelper binning(8., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearFineLowerStrong(){
  binninghelper binning(12., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearFineUpperLoose(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {190., 10.}, {205., 15.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearFineUpperStrong(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {190., 10.}, {195., 5,}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300LowerLoose(){
  binninghelper binning(8., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {300., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300LowerStrong(){
  binninghelper binning(12., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {300., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300UpperLoose(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {280., 20.}, {305., 25.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300UpperStrong(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {280., 20.}, {295., 15.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240LowerLoose(){
  binninghelper binning(8., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {240., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240LowerStrong(){
  binninghelper binning(12., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {240., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240UpperLoose(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {220., 20.}, {245., 25.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240UpperStrong(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {220., 20.}, {235., 15.}});
  return binning.CreateCombinedBinning();
}

std::vector<double>getJetPtBinningNonLinSmearTrunc(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"lowerloose", getJetPtBinningNonLinSmearLowerLoose},
    {"lowerstrong", getJetPtBinningNonLinSmearLowerStrong},
    {"upperloose", getJetPtBinningNonLinSmearUpperLoose},
    {"upperstrong", getJetPtBinningNonLinSmearUpperStrong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}

std::vector<double>getJetPtBinningNonLinSmearLargeTrunc(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"lowerloose", getJetPtBinningNonLinSmearLargeLowerLoose},
    {"lowerstrong", getJetPtBinningNonLinSmearLargeLowerStrong},
    {"upperloose", getJetPtBinningNonLinSmearLargeUpperLoose},
    {"upperstrong", getJetPtBinningNonLinSmearLargeUpperStrong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}

std::vector<double>getJetPtBinningNonLinSmearFineTrunc(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"lowerloose", getJetPtBinningNonLinSmearFineLowerLoose},
    {"lowerstrong", getJetPtBinningNonLinSmearFineLowerStrong},
    {"upperloose", getJetPtBinningNonLinSmearFineUpperLoose},
    {"upperstrong", getJetPtBinningNonLinSmearFineUpperStrong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}

std::vector<double>getJetPtBinningNonLinSmearUltra300Trunc(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"lowerloose", getJetPtBinningNonLinSmearUltra300LowerLoose},
    {"lowerstrong", getJetPtBinningNonLinSmearUltra300LowerStrong},
    {"upperloose", getJetPtBinningNonLinSmearUltra300UpperLoose},
    {"upperstrong", getJetPtBinningNonLinSmearUltra300UpperStrong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}

std::vector<double>getJetPtBinningNonLinSmearUltra240Trunc(const std::string_view option) {
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
    {"lowerloose", getJetPtBinningNonLinSmearUltra240LowerLoose},
    {"lowerstrong", getJetPtBinningNonLinSmearUltra240LowerStrong},
    {"upperloose", getJetPtBinningNonLinSmearUltra240UpperLoose},
    {"upperstrong", getJetPtBinningNonLinSmearUltra240UpperStrong}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second();
}