#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

TH1 *getEfficiency(TFile &reader, const std::string_view rstring) {
  TH1 *result(nullptr);
  reader.cd(rstring.data());
  gDirectory->cd("response");
  result = static_cast<TH1 *>(gDirectory->Get(Form("effKine_%s", rstring.data())));
  result->SetDirectory(nullptr);
  return result;
}

void makeKinematicEfficiency1D_SpectrumTask(const std::string_view filename = "correctedSVD.root"){
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));

  auto plot = new ROOT6tools::TSavableCanvas("effKine", "Kinematic efficiencies jet spectrum", 800, 600);
  plot->cd();
  (new ROOT6tools::TAxisFrame("effFrame", "p_{t} (GeV/c)", "#epsilon_{kine}", 0., 400., 0., 1.1))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.7, 0.15, 0.89, 0.5);
  leg->Draw();
  std::map<double, Style> rstyles = {{0.2, {kBlue, 24}}, {0.3, {kRed, 25}}, {0.4, {kGreen, 26}}, {0.5, {kOrange, 27}}, {0.6, {kViolet, 28}}};
  
  for(auto rdir : *reader->GetListOfKeys()){
    std::string rstring(rdir->GetName());
    auto rval = double(std::stoi(rstring.substr(1)))/10.;
    auto effhist = getEfficiency(*reader, rstring);
    rstyles[rval].SetStyle<TH1>(*effhist);
    effhist->Draw("epsame");
    leg->AddEntry(effhist, Form("R=%.1f", rval), "epsame");
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}
