#ifndef __CLING__
#include <map>
#include <vector>
#include <string>
#include <RStringView.h>
#endif

std::vector<double> getZgBinningCoarse(){
  std::vector<double> result;
  result.push_back(0.);
  for(double d = 0.1; d <= 0.5; d += 0.1) result.push_back(d);
  return result;
}

std::vector<double> getMinBiasPtBinningDet_Opt1() {
  // default: {20., 25., 30., 35., 40., 50., 60., 80., 120.}
  // option1: Finer at low pt, larger at high pt
  std::vector<double> result = {20., 24., 28., 32., 38., 46., 54., 72., 120.};
  return result;
}

std::vector<double> getMinBiasPtBinningDet_Opt2() {
  // default: {20., 25., 30., 35., 40., 50., 60., 80., 120.}
  // option2: Larger at low pt, finer at high pt
  std::vector<double> result = {20., 28., 35., 47., 55., 63., 75., 90., 120.};
  return result;
}

std::vector<double> getMinBiasPtBinningDet_Opt3() {
  // default: {20., 25., 30., 35., 40., 50., 60., 80., 120.}
  // option3: Shifted  back by 2 GeV 
  std::vector<double> result = {20., 27., 32., 37., 42., 52., 62., 82., 120.};
  return result;
}

std::vector<double> getMinBiasPtBinningDet_Opt4() {
  // default: {20., 25., 30., 35., 40., 50., 60., 80., 120.}
  // option4: Shifted forth  by 2 GeV 
  std::vector<double> result = {20., 23., 28., 33., 38., 38., 58., 78., 120.};
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

std::vector<double> getEJ1PtBinningDet_Opt1(){
  // default: {80., 85., 90., 95., 100., 110., 120., 140., 160., 200.}
  // option1: Finer at low pt, larger at high pt
  std::vector<double> result = {80., 83., 87., 92., 96., 102., 114., 134., 155., 200.};
  return result;
}

std::vector<double> getEJ1PtBinningDet_Opt2(){
  // default: {80., 85., 90., 95., 100., 110., 120., 140., 160., 200.}
  // option2: Larger at low pt, finer at high pt
  std::vector<double> result = {80., 87., 93., 99., 108., 120., 130., 145., 170., 200.};
  return result;
}

std::vector<double> getEJ1PtBinningDet_Opt3(){
  // default: {80., 85., 90., 95., 100., 110., 120., 140., 160., 200.}
  // option3: Shifted  back by 2 GeV 
  std::vector<double> result = {80., 83., 88., 93., 98., 108., 118., 138., 158., 200.};
  return result;
}

std::vector<double> getEJ1PtBinningDet_Opt4(){
  // default: {80., 85., 90., 95., 100., 110., 120., 140., 160., 200.}
  // option4: Shifted forth  by 2 GeV 
  std::vector<double> result = {80., 87., 92., 97., 102., 112., 122., 142., 162., 200.};
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

std::vector<double> getEJ2PtBinningDet_Opt1(){
  // default: {60., 65., 70., 85., 90., 100., 120., 160.}
  // option1: Finer at low pt, larger at high pt
  std::vector<double> result = {60., 63., 68., 75., 84., 96., 112., 160.};
  return result;
}

std::vector<double> getEJ2PtBinningDet_Opt2(){
  // default: {60., 65., 70., 85., 90., 100., 120., 160.}
  // option1: Finer at low pt, larger at high pt
  std::vector<double> result = {60., 70., 80., 91., 102., 114., 128., 160.};
  return result;
}

std::vector<double> getEJ2PtBinningDet_Opt3(){
  // default: {60., 65., 70., 85., 90., 100., 120., 160.}
  // option3: Shifted  back by 2 GeV 
  std::vector<double> result = {60., 67., 72., 82., 92., 102., 122., 160.};
  return result;
}

std::vector<double> getEJ2PtBinningDet_Opt4(){
  // default: {60., 65., 70., 85., 90., 100., 120., 160.}
  // option4: Shifted forth  by 2 GeV 
  std::vector<double> result = {60., 63., 68., 78., 88., 98., 118., 160.};
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

std::vector<double> getPtBinningRealistic(const std::string_view trigger, const std::string_view binoption) {
  if(trigger == "INT7") {
    std::map<std::string, std::function<std::vector<double> ()>> binhandler = {{"option1", getMinBiasPtBinningDet_Opt1}, 
                                                                               {"option2", getMinBiasPtBinningDet_Opt2},
                                                                               {"option3", getMinBiasPtBinningDet_Opt3},
                                                                               {"option4", getMinBiasPtBinningDet_Opt4}};
    return binhandler[static_cast<std::string>(binoption)]();
  } else if(trigger == "EJ2") {
    std::map<std::string, std::function<std::vector<double> ()>> binhandler = {{"option1", getEJ2PtBinningDet_Opt1}, 
                                                                               {"option2", getEJ2PtBinningDet_Opt2},
                                                                               {"option3", getEJ2PtBinningDet_Opt3},
                                                                               {"option4", getEJ2PtBinningDet_Opt4}};
    return binhandler[static_cast<std::string>(binoption)]();
  } else if(trigger == "EJ1") {
    std::map<std::string, std::function<std::vector<double> ()>> binhandler = {{"option1", getEJ1PtBinningDet_Opt1}, 
                                                                               {"option2", getEJ1PtBinningDet_Opt2},
                                                                               {"option3", getEJ1PtBinningDet_Opt3},
                                                                               {"option4", getEJ1PtBinningDet_Opt4}};
    return binhandler[static_cast<std::string>(binoption)]();
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
