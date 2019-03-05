#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/string.C"

std::set<std::string> getListOfPeriods(const std::string_view inputdir){
  std::set<std::string> result;
  for(auto d : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data())){
    if(d.find("LHC") != std::string::npos){
      if(!gSystem->AccessPathName(Form("%s/%s/trackingEfficiency_full.root", inputdir.data(), d.data()))) result.insert(d);
    }
  }
  return result;
}

TH1 *readEfficiency(const std::string_view filename) {
  TH1 *result(nullptr);
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  result = static_cast<TH1 *>(reader->Get("efficiency"));
  result->SetDirectory(nullptr);
  return result;
}

void compareTrackingEffPeriods() {
  auto plot = new ROOT6tools::TSavableCanvas("comparisonPeriods", "Comparison periods", 800, 600);
  plot->cd();
  (new ROOT6tools::TAxisFrame("effframe", "p_{t,part} (GeV/c)", "Tracking efficiency", 0., 200., 0., 1.2))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.75, 0.89, 0.89);
  leg->SetNColumns(4);
  leg->Draw();

  Color_t colors[9] = {kRed, kBlue, kGreen, kMagenta, kOrange, kTeal, kCyan, kGray, kViolet};
  Style_t markers[7] = {24, 25, 26, 27, 28, 29, 30};
  std::string workdir = gSystem->GetWorkingDirectory();
  int icol(0), imark(0);
  for(auto p : getListOfPeriods(workdir)){
    auto eff = readEfficiency(Form("%s/%s/trackingEfficiency_full.root", workdir.data(), p.data()));
    eff->SetName(Form("efficiency_%s", p.data()));
    Style{colors[icol++], markers[imark++]}.SetStyle<TH1>(*eff);
    eff->Draw("psame");
    leg->AddEntry(eff, p.data(), "lep");
    icol = icol == 9 ? 0 : icol;
    imark = imark == 7 ? 0 : imark;
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}