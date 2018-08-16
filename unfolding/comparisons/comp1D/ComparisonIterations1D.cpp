#ifndef __CLING__
#include <array>
#include <map>
#include <memory>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>

#include <TAxisFrame.h>
#include <TNDCLabel.h>
#include <TDefaultLegend.h>
#include <TSavableCanvas.h>
#endif

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"
#include "../../../helpers/root.C"
#include "../../../helpers/string.C"

struct JetDef {
  std::string fJetType;
  double fJetRadius;
  std::string fTrigger;
};

std::string getFileTag(const std::string_view infile){
  const char *tagremove = "unfoldedEnergyBayes_";
  std::string filetag = basename(infile);
  filetag.erase(filetag.find(tagremove), strlen(tagremove));
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

JetDef getJetType(const std::string_view filetag) {
  auto tokens = tokenize(std::string(filetag), '_');
  return {tokens[0], double(std::stoi(tokens[1].substr(1)))/10., tokens[2]};
}

void ComparisonIterations1D(const std::string_view inputfile){
  auto tag = getFileTag(inputfile);
  auto jd = getJetType(tag);
  auto canvas = new ROOT6tools::TSavableCanvas(Form("comparisonIterationsEnergyBayes_%s", tag.data()), "Comparison iterations bayesian unfolding", 1200, 600);
  canvas->Divide(2,1);

  canvas->cd(1);
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "dN/dp_{t} (GeV/c)", 0., 200., 1e-1, 1e7))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.45, 0.89, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.6, 0.89, 0.89);
  leg->Draw();

  canvas->cd(2);
  (new ROOT6tools::TAxisFrame("ratioframe", "p_{t} (GeV/c)", "ratio to iter=4", 0., 200., 0., 2.))->Draw("axis");

  std::array<Color_t, 7> colors = {{kBlack, kMagenta, kGreen, kRed, kBlue, kOrange, kTeal}};
  std::array<Style_t, 7> markers = {{24, 25, 26, 27, 28, 29, 30}};

  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  std::map<int, TH1 *> normalizedRaw;
  canvas->cd(1);
  for(auto iter : ROOT::TSeqI(1,7)){
    reader->cd(Form("iteration%d", iter));
    auto spec = static_cast<TH1 *>(gDirectory->Get(Form("unfolded_iter%d", iter)));
    spec->SetDirectory(nullptr);
    normalizeBinWidth(spec);
    Style{colors[iter-1], markers[iter-1]}.SetStyle<TH1>(*spec);
    spec->Draw("epsame");
    leg->AddEntry(spec, Form("iteration=%d", iter), "lep");
    normalizedRaw[iter] = spec;
  }

  canvas->cd(2);
  auto ref = normalizedRaw[4];
  for(auto v : normalizedRaw){
    if(v.first == 4) continue;
    auto ratio = histcopy(v.second);
    ratio->SetDirectory(nullptr);
    ratio->SetName(Form("Ratio_%d_4", v.first));
    ratio->Divide(ref);
    ratio->Draw("epsame");
  }
  canvas->cd();
  canvas->Update();
  canvas->SaveCanvas(canvas->GetName());
}