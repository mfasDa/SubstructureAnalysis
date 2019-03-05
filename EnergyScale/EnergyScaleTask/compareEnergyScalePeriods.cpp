#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/filesystem.C"
#include "../../helpers/graphics.C"
#include "../../helpers/string.C"

const double kVerySmall = 1e-5;

struct EnergyScaleResults {
  double fR;
  TGraph * fMean;
  TGraph * fMedian;
  TGraph * fResolution;

  bool operator==(const EnergyScaleResults &other) const { return TMath::Abs(fR - other.fR) < kVerySmall; }
  bool operator<(const EnergyScaleResults &other) const { return fR < other.fR; }
};

TGraph *convertGraph(TGraphErrors *in) {
  TGraph *out = new TGraph;
  for(auto p : ROOT::TSeqI(0, in->GetN())){
    out->SetPoint(p, in->GetX()[p], in->GetY()[p]);
  }
  return out;
}

std::set<std::string> findPeriods(const std::string_view inputdir) {
  std::set<std::string> periods;
  for(auto p : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data())){
    if(p.length() > 6) continue;
    if(p.find("LHC") == std::string::npos) continue;
    if(gSystem->AccessPathName(Form("%s/%s/EnergyScaleResults.root", inputdir.data(), p.data()))) continue;
    periods.insert(p);
  }  
  return periods;
}

std::set<EnergyScaleResults> readFile(const std::string_view filename) {
  std::set<EnergyScaleResults> result;
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  for(double r = 0.2; r < 0.6; r+=0.1) {
    TGraphErrors * mean = static_cast<TGraphErrors *>(reader->Get(Form("EnergyScale_R%02d_mean", int(r*10.)))),
           * median = static_cast<TGraphErrors *>(reader->Get(Form("EnergyScale_R%02d_median", int(r*10.)))),
           * width = static_cast<TGraphErrors *>(reader->Get(Form("EnergyScale_R%02d_width", int(r*10.))));
    result.insert({r, mean, median, width});
    result.insert({r, convertGraph(mean), convertGraph(median), convertGraph(width)});
  }
  return result; 
}

void compareEnergyScalePeriods() {
  std::map<std::string, std::set<EnergyScaleResults>> data;
  for(const auto &p : findPeriods(gSystem->GetWorkingDirectory())) data[p] = readFile(Form("%s/%s/EnergyScaleResults.root", gSystem->GetWorkingDirectory().data(), p.data()));
  
  auto plot = new ROOT6tools::TSavableCanvas("periodComparisonEnergyScale", "Period comparison jet energy scale", 1200, 800);
  plot->Divide(4, 2);
  
  int irow = 0;
  std::map<std::string, Color_t> periodColors;
  std::map<std::string, Style_t> periodStyles;
  std::vector<Color_t> colors = {kRed, kBlue, kGreen, kOrange, kViolet, kTeal, kGray, kMagenta};
  std::vector<Style_t> markers = {24, 25, 26, 27, 28};
  auto currentcolor = colors.begin();
  auto currentmarker = markers.begin();
  for(double r = 0.2; r < 0.6; r+=0.1){
    plot->cd(irow + 1);
    (new ROOT6tools::TAxisFrame(Form("meanframe_R%02d", int(r*10.)), "p_{t} (GeV/c)", "|(p_{t,det} - p_{t,part})/p_{t,part}|",  0., 200., -0.5, 0.2))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.3, 0.22, Form("R=%.1f", r)))->Draw();
    TLegend *leg(nullptr);
    if(!irow) {
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.5, 0.89, 0.89);
      leg->Draw();
    }
    for(auto [p, d] : data){
      auto rdata = d.find({r, nullptr, nullptr, nullptr});
      if(rdata != d.end()) {
        Color_t mycolor;
        Style_t mymarker;
        auto storedcolor = periodColors.find(p);
        auto storedmarker = periodStyles.find(p);
        if(storedcolor != periodColors.end()) {
          mycolor = storedcolor->second;
        } else {
          mycolor = *currentcolor;
          periodColors[p] = mycolor;
          currentcolor++;
          if(currentcolor == colors.end()) currentcolor = colors.begin();
        }
        if(storedmarker != periodStyles.end()) {
          mymarker = storedmarker->second;
        } else {
          mymarker = *currentmarker;
          periodStyles[p] = mymarker;
          currentmarker++;
          if(currentmarker == markers.end()) currentmarker = markers.begin();
        }
        Style{mycolor, mymarker}.SetStyle<TGraph>(*rdata->fMean);
        rdata->fMean->Draw("epsame");
        if(leg) leg->AddEntry(rdata->fMean, p.data(), "lep");
      }
    }
    
    plot->cd(irow + 5);
    (new ROOT6tools::TAxisFrame(Form("widthframe_R%02d", int(r*10.)), "p_{t} (GeV/c)", "#sigma((p_{t,det} - p_{t,part})/p_{t,part}0",  0., 200., 0., 0.4))->Draw("axis");
    for(auto [p,d] : data) {
      auto rdata = d.find({r, nullptr, nullptr, nullptr});
      if(rdata != d.end()) {
        Style{periodColors.find(p)->second, periodStyles.find(p)->second}.SetStyle(*rdata->fResolution);
        rdata->fResolution->Draw("epsame");
      }
    }
    irow++;
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}