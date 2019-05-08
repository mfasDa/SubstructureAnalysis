#include "../../meta/stl.C"
#include "../../meta/root.C"

std::vector<double> getJetPtBinningNonLinSmearPoorOption1(){
  return { 10.0, 11.0, 12.0, 14.0, 16.0, 18.0, 20.0, 24.0, 28.0, 37.0, 45.0, 54.0, 65.0, 69.0, 73.0, 77.0, 81.0, 86.0, 90.0, 94.0, 98.0, 103.0, 109.0, 117.0, 127.0, 137.0, 147.0, 159.0, 171.0, 183.0, 197.0, 218.0, 240.0};
}

std::vector<double> getJetPtBinningNonLinSmearPoorOption2(){
  return { 10.0, 12.0, 14.0, 16.0, 19.0, 22.0, 25.0, 29.0, 38.0, 50.0, 62.0, 67.0, 72.0, 78.0, 86.0, 92.0, 96.0, 101.0, 107.0, 114.0, 122.0, 130.0, 140.0, 142.0, 154.0, 168.0, 180.0, 189.0, 197.0, 206.0, 214.0, 227.0, 240.0};
}

std::vector<double> getJetPtBinningNonLinSmearPoorOption3(){
  return { 10.0, 13.0, 15.0, 17.0, 19.0, 21.0, 26.0, 31.0, 41.0, 51.0, 61.0, 66.0, 71.0, 76.0, 81.0, 86.0, 91.0, 96.0, 101.0, 106.0, 111.0, 116.0, 121.0, 131.0, 141.0, 151.0, 161.0, 171.0, 181.0, 191.0, 201.0, 221.0, 240.0};
}

std::vector<double> getJetPtBinningNonLinSmearPoorOption4(){
  return { 10.0, 11.0, 13.0, 15.0, 17.0, 19.0, 24.0, 29.0, 39.0, 49.0, 59.0, 64.0, 69.0, 74.0, 79.0, 84.0, 89.0, 94.0, 99.0, 104.0, 109.0, 114.0, 119.0, 129.0, 139.0, 149.0, 159.0, 169.0, 179.0, 189.0, 199.0, 219.0, 240.0};
}

std::vector<double> getJetPtBinningNonLinSmearPoorrBV(const std::string_view option){
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
      {"option1", getJetPtBinningNonLinSmearPoorOption1},  
      {"option2", getJetPtBinningNonLinSmearPoorOption2},
      {"option3", getJetPtBinningNonLinSmearPoorOption3},
      {"option4", getJetPtBinningNonLinSmearPoorOption4}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second(); 
}

std::vector<double> getJetPtBinningNonLinSmearOption1(){
  return { 10., 11., 12., 13., 14., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33, 36., 39., 42., 46., 52., 58., 68., 78., 88., 100., 115., 140.};
}

std::vector<double> getJetPtBinningNonLinSmearOption2(){
  return { 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 31., 34., 37., 40., 45., 50., 56., 60., 65., 71., 77., 85., 93., 103., 113., 125., 140.};
}

std::vector<double> getJetPtBinningNonLinSmearOption3(){
  return { 10., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 41., 46., 51., 56., 61., 71., 81., 91., 101., 111., 121., 140.};
}

std::vector<double> getJetPtBinningNonLinSmearOption4(){
  return { 10., 11., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 44., 49., 54., 59., 69., 79., 89., 99., 109., 119., 140.};
}

std::vector<double> getJetPtBinningNonLinSmearBV(const std::string_view option){
  std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
      {"option1", getJetPtBinningNonLinSmearOption1},  
      {"option2", getJetPtBinningNonLinSmearOption2},
      {"option3", getJetPtBinningNonLinSmearOption3},
      {"option4", getJetPtBinningNonLinSmearOption4}
  };
  auto functor = functors.find(std::string(option));
  if(functor == functors.end()) return {};
  return functor->second(); 
}

std::vector<double> getJetPtBinningNonLinSmearLargeOption1(){
  return { 10., 11., 12., 13., 14., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 36., 39., 42., 46., 52., 58., 68., 78., 88., 98., 108., 125., 145., 170., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeOption2(){
  return { 10., 12., 14., 16., 18., 20., 22., 25., 28., 31., 34., 38., 42., 47., 52., 58., 64., 71., 78., 86., 94., 105., 114., 123., 133., 144., 155., 167., 183., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeOption3(){
  return { 10., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 41., 46., 51., 56., 61., 71., 81., 91., 101., 111., 121., 141., 161., 181., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeOption4(){
  return { 10., 11., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 44., 49., 54., 59., 69., 79., 89., 99., 109., 119., 139., 159., 179., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeBV(const std::string_view option){
    std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
      {"option1", getJetPtBinningNonLinSmearLargeOption1},  
      {"option2", getJetPtBinningNonLinSmearLargeOption2},
      {"option3", getJetPtBinningNonLinSmearLargeOption3},
      {"option4", getJetPtBinningNonLinSmearLargeOption4}
    };
    auto functor = functors.find(std::string(option));
    if(functor == functors.end()) return {};
    return functor->second();
}

std::vector<double> getJetPtBinningNonLinSmearLargeFineOption1(){
  return { 10., 11., 12., 13., 14., 16., 17., 19., 21., 23., 25., 27., 29., 32., 35., 38., 41., 44., 47., 50., 53., 56., 59., 62., 65., 69., 73., 78., 85., 92., 99., 106., 114., 122., 131., 139., 148., 158., 170., 184., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeFineOption2(){
  return { 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 31., 34., 37., 40., 44., 48., 52., 56., 60., 65., 70., 75., 80., 85., 90., 95., 100., 106., 112., 118., 124., 131., 138., 145., 152., 160., 168., 176., 184., 192., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeFineOption3(){
  return { 10., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 41., 45., 49., 53., 57., 61., 66., 71., 76., 81., 86., 91., 96., 101., 106., 111., 116., 121., 131., 141., 151., 161., 171., 181., 191., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeFineOption4(){
  return { 10., 11., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 43., 47., 51., 55., 59., 64., 69., 74., 79., 84., 89., 94., 99., 104., 109., 114., 119., 129., 139., 149., 159., 169., 179., 189., 200.};
}

std::vector<double> getJetPtBinningNonLinSmearLargeFineBV(const std::string_view option){
    std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
      {"option1", getJetPtBinningNonLinSmearLargeFineOption1},  
      {"option2", getJetPtBinningNonLinSmearLargeFineOption2},
      {"option3", getJetPtBinningNonLinSmearLargeFineOption3},
      {"option4", getJetPtBinningNonLinSmearLargeFineOption4}
    };
    auto functor = functors.find(std::string(option));
    if(functor == functors.end()) return {};
    return functor->second();
}

std::vector<double> getJetPtBinningNonLinSmearUltra240Option1(){
  return { 10., 11., 12., 13., 14., 15., 17., 19., 21., 23., 25., 27., 29., 32., 35., 38., 41., 44., 47., 50., 53., 56., 59., 62., 65., 69., 73., 78., 85., 92., 99., 106., 114., 122., 131., 139., 148., 158., 170., 184., 200., 218., 240.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra240Option2(){
  return { 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 31., 34., 37., 40., 44., 48., 52., 56., 60., 65., 70., 75., 80., 85., 90., 95., 100., 106., 112., 118., 124., 131., 138., 145., 152., 160., 168., 176., 184., 194., 207., 222., 240.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra240Option3(){
  return { 10., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 41., 45., 49., 53., 57., 61., 66., 71., 76., 81., 86., 91., 96., 101., 106., 111., 116., 121., 131., 141., 151., 161., 171., 181., 191., 201., 221., 240.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra240Option4(){
  return { 10., 11., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 43., 47., 51., 55., 59., 64., 69., 74., 79., 84., 89., 94., 99., 104., 109., 114., 119., 129., 139., 149., 159., 169., 179., 189., 199., 219., 240.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra240BV(const std::string_view option){
    std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
      {"option1", getJetPtBinningNonLinSmearUltra240Option1},  
      {"option2", getJetPtBinningNonLinSmearUltra240Option2},
      {"option3", getJetPtBinningNonLinSmearUltra240Option3},
      {"option4", getJetPtBinningNonLinSmearUltra240Option4}
    };
    auto functor = functors.find(std::string(option));
    if(functor == functors.end()) return {};
    return functor->second();
}

std::vector<double> getJetPtBinningNonLinSmearUltra300Option1(){
  return { 10., 11., 12., 13., 14., 15., 17., 19., 21., 23., 25., 27., 29., 32., 35., 38., 41., 44., 47., 50., 53., 56., 59., 62., 65., 69., 73., 78., 85., 92., 99., 106., 114., 122., 131., 139., 148., 158., 170., 184., 200., 218., 240., 260., 280, 300.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra300Option2(){
  return { 10., 12., 14., 16., 18., 20., 22., 24., 26., 28., 31., 34., 37., 40., 44., 48., 52., 56., 60., 65., 70., 75., 80., 85., 90., 95., 100., 106., 112., 118., 124., 131., 138., 145., 152., 160., 168., 176., 184., 194., 210., 228., 246., 264., 282., 300.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra300Option3(){
  return { 10., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 41., 45., 49., 53., 57., 61., 66., 71., 76., 81., 86., 91., 96., 101., 106., 111., 116., 121., 131., 141., 151., 161., 171., 181., 191., 201., 221., 241., 261., 281., 300.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra300Option4(){
  return { 10., 11., 13., 15., 17., 19., 21., 23., 25., 27., 29., 31., 33., 35., 37., 39., 43., 47., 51., 55., 59., 64., 69., 74., 79., 84., 89., 94., 99., 104., 109., 114., 119., 129., 139., 149., 159., 169., 179., 189., 199., 219., 240.};
}

std::vector<double> getJetPtBinningNonLinSmearUltra300BV(const std::string_view option){
    std::unordered_map<std::string, std::function<std::vector<double>()>> functors = {
      {"option1", getJetPtBinningNonLinSmearUltra300Option1},  
      {"option2", getJetPtBinningNonLinSmearUltra300Option2},
      {"option3", getJetPtBinningNonLinSmearUltra300Option3},
      {"option4", getJetPtBinningNonLinSmearUltra300Option4}
    };
    auto functor = functors.find(std::string(option));
    if(functor == functors.end()) return {};
    return functor->second();
}