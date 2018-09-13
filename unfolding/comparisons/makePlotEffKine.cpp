#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"

struct unfoldconfig {
  std::string fJetType;
  double fR;
  std::string fTrigger;
  std::string fObservable;
};

unfoldconfig extractFileTokens(const std::string_view filename){
  auto tokens = tokenize(std::string(filename.substr(0, filename.find(".root"))), '_');
  return {tokens[1], double(std::stoi(tokens[2].substr(1,2)))/10., tokens[3], tokens[5]};
}

std::pair<double, double> extractPt(const std::string_view histname){
  auto tokens = tokenize(std::string(histname), '_');
  return {std::stod(tokens[1]), std::stod(tokens[2])};
}

std::vector<TH1 *> getefficiencies(const std::string_view inputfile){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  std::vector<TH1 *> result;
  for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())){
    if(contains(k->GetName(), "efficiency")){
      auto h = k->ReadObject<TH1>();
      h->SetDirectory(nullptr);
      result.push_back(h);
    } 
  }
  std::sort(result.begin(), result.end(), [](const TH1 *first, const TH1 *second) { return extractPt(first->GetName()).first < extractPt(second->GetName()).first;});
  return result;
}

void makePlotEffKine(const std::string_view inputfile){
  auto efficiencies = getefficiencies(inputfile);
  auto conf = extractFileTokens(inputfile);

  auto plot = new ROOT6tools::TSavableCanvas(Form("EffKine_%s_R%02d_%s_%s", conf.fJetType.data(), int(conf.fR * 10.), conf.fTrigger.data(), conf.fObservable.data()),
                                             Form("Kinematic efficiencies %s %s, R=%.1f, %s", conf.fObservable.data(), conf.fJetType.data(), conf.fR, conf.fTrigger.data()),
                                             1000, 600);
  plot->cd();
  gPad->SetRightMargin(0.35);
  (new ROOT6tools::TAxisFrame("effframe", "z_g", "Kinematic efficiency", 0., 0.5, 0., 1.))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.5, 0.89, 0.89);
  (new ROOT6tools::TNDCLabel(0.15, 0.9, 0.5, 0.95, Form("%s, R=%.1f, %s", conf.fJetType.data(), conf.fR, conf.fTrigger.data())))->Draw();
  leg->Draw();

  std::array<Color_t, 15> colors = {{kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kAzure, kGray, kMagenta, kCyan, kRed-6, kBlue-6, kGreen-6, kOrange-6, kViolet -6}};
  std::array<Style_t, 15> markers = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38}};

  int ieff = 0;
  for(auto e : efficiencies) {
    Style{colors[ieff], markers[ieff]}.SetStyle<TH1>(*e);
    e->Draw("epsame");
    auto range = extractPt(e->GetName()); 
    leg->AddEntry(e, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", range.first, range.second), "lep");
    ieff++;
  }

  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}