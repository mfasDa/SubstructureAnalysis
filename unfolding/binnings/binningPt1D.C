#include "binninghelper.cpp"

std::vector<double> getJetPtBinningNonLinSmearMinBias(){
  binninghelper binning(10., {{40., 2.}, {60., 5.}, {80., 5.}, {120., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearEMCAL(){
  binninghelper binning(52., {{60., 4.}, {120., 5.}, {200., 10.}, {240., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearMinBiasPoor(){
  binninghelper binning(10., {{40., 5.}, {60., 10.}, {120., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearPoor(){
  binninghelper binning(10., {{40., 5.}, {60., 10.}, {120., 20.}, {240., 40.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmear(){
  binninghelper binning(10., {{40., 2}, {60., 5.}, {120., 10.}, {140., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeLow(){
  binninghelper binning(10., {{40., 2.}, {60., 5.}, {120., 10.}, {200., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearLargeFineLow(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300(){
  binninghelper.binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {300., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240(){
  binninghelper binning(10., {{40., 2.}, {60., 4.}, {120., 5.}, {200., 10.}, {240., 20.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrueMinBias(){
  binninghelper binning(5., {{10., 5.}, {80. 10.}, {160., 20.}, {300., 140.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrueMinBiasPoor(){
  binninghelper binning(5. {{10., 5.}, {40., 10.}, {80., 20.}, {160., 40.}, {300., 140.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrueEMCAL(){
  binninghelper binning(5., {{60., 55.}, {140., 10.}, {200., 20.}, {320., 40.}, {500., 180.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTruePoor(){
  binninghelper binning(5., {{10., 5.}, {40., 10.}, {80., 20.}, {160., 40.}, {320., 80.}, {600., 280.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrue(){
  binninghelper binning(5., {{10., 5.}, {80., 10.}, {180, 20.}, {300., 120.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrueLargeLow(){
  binninghelper binning(5., {{10., 5.}, {80., 10.}, {200., 20.}, {280., 40.}, {500., 220.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrueLargeFineLow(){
  binninghelper binning(5., {{10., 5.}, {60., 5.}, {140., 10.}, {200., 20.}, {280., 40.}, {500., 220.}})
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrueUltra300(){
  binninghelper binning(5., {{10., 5.}, {60., 5.}, {140., 10.}, {200., 20.}, {520.. 40.}, {600., 80.}});
  return binning.CreateCombinedBinning();
}

std::vector<double> getJetPtBinningNonLinTrueUltra240(){
  binninghelper binning(5., {{10., 5.}, {60., 5.}, {140., 10.}, {200., 20.}, {320., 40.}, {500., 180.}});
  return binning.CreateCombinedBinning();
}
