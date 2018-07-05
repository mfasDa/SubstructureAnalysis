#ifndef __CLING__
#include <algorithm>
#include <memory>
#include <string>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH2.h>
#include <TKey.h>

#include <TAxisFrame.h>
#include <TDefaultLegend.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

#include "../../helpers/graphics.C"
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

TH1 *makePtProjection(const TH2 &inputhist, int ptbin) {
  double ptmin = inputhist.GetYaxis()->GetBinLowEdge(ptbin+1),
         ptmax = inputhist.GetYaxis()->GetBinUpEdge(ptbin+1);
  auto result = inputhist.ProjectionX(Form("%s_%d_%d", inputhist.GetName(), int(ptmin), int(ptmax)), ptbin+1, ptbin+1);
  result->SetDirectory(nullptr);
  result->Scale(1./result->Integral());   // Transform to per-jet yield
  return result;
}

void makeComparisonUnfoldedRaw(const std::string_view inputfile) {
  std::unique_ptr<TH2> hraw;
  std::map<int, std::unique_ptr<TH2>> hfold;
  {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    hraw = std::unique_ptr<TH2>(static_cast<TH2 *>(reader->Get("hraw")));
    hraw->SetDirectory(nullptr);

    std::vector<int> iterations = {1, 4, 10, 15, 20, 25, 30, 35};
    for(auto i : iterations) {
      reader->cd(Form("iteration%d", i));
      auto keys = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
      auto foldhist = (*std::find_if(keys.begin(), keys.end(), [](const TKey *k) { return contains(k->GetName(), "_unfolded_");}))->ReadObject<TH2>();
      foldhist->SetDirectory(nullptr);
      hfold[i] = std::unique_ptr<TH2>(foldhist);
    } 
  }
  int ndists = hraw->GetYaxis()->GetNbins();
  auto distmin = hraw->GetXaxis()->GetBinLowEdge(1), distmax = hraw->GetXaxis()->GetBinLowEdge(hraw->GetXaxis()->GetNbins());
  auto conf = extractFileTokens(inputfile);

  auto compplot = new ROOT6tools::TSavableCanvas(Form("CompUnfoldRaw_%s_R%02d_%s_%s", conf.fJetType.data(), int(conf.fR * 10.), conf.fTrigger.data(), conf.fObservable.data()),
                                                 Form("Comparison Folder/Raw %s %s, R=%.1f, %s", conf.fObservable.data(), conf.fJetType.data(), conf.fR, conf.fTrigger.data()),
                                                 1200, 1000);
  compplot->DivideSquare(ndists);

  Style rawstyle{kBlack, 20};
  std::vector<Style> iterstyles = {{kRed, 24}, {kBlue, 25}, {kGreen, 26}, {kViolet, 27}, {kOrange, 28}, {kTeal, 29}, {kAzure, 30}, {kMagenta, 31}, {kGray,32}};

  for(auto ptbin : ROOT::TSeqI(0, ndists)){
    compplot->cd(ptbin+1);
    TLegend *leg(nullptr);
    (new ROOT6tools::TAxisFrame(Form("compframe_%s_R%02d_%s_%s", conf.fJetType.data(), int(conf.fR * 10.), conf.fTrigger.data(), conf.fObservable.data()), conf.fObservable.data(), Form("1/N_{jet} dN/d%s", conf.fObservable.data()), distmin, distmax, 0, 0.5))->Draw("axis");
    if(!ptbin){
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.5, 0.89, 0.89);
      leg->Draw();
      (new ROOT6tools::TNDCLabel(0.15, 0.7, 0.5, 0.79, Form("%s, R=%.1f, %s", conf.fJetType.data(), conf.fR, conf.fTrigger.data())))->Draw();
    }
    auto ptmin = hraw->GetYaxis()->GetBinLowEdge(ptbin+1), ptmax = hraw->GetYaxis()->GetBinUpEdge(ptbin+1);
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.5, 0.89, Form("%.1f GeV/c < p_{t,j,d} < %.1f GeV/c", ptmin, ptmax)))->Draw();
    auto rawslice = makePtProjection(*hraw, ptbin);
    rawstyle.SetStyle<decltype(*rawslice)>(*rawslice);
    rawslice->Draw("epsame");
    auto iiter = 0;
    for(const auto &fold : hfold) {
      auto foldslice = makePtProjection(*fold.second, ptbin);
      iterstyles[iiter++].SetStyle<decltype(*foldslice)>(*foldslice);
      foldslice->Draw("epsame");
      if(leg) leg->AddEntry(foldslice, Form("unfolded, iteration %d", fold.first), "lep");
    }
  }
  compplot->cd();
  compplot->Update();

  // Also draw ratios
  auto ratioplot = new ROOT6tools::TSavableCanvas(Form("RatioUnfoldRaw_%s_R%02d_%s_%s", conf.fJetType.data(), int(conf.fR * 10.), conf.fTrigger.data(), conf.fObservable.data()),
                                                  Form("Ratio Unfolded/Raw %s %s, R=%.1f, %s", conf.fObservable.data(), conf.fJetType.data(), conf.fR, conf.fTrigger.data()),
                                                  1200, 1000);
  ratioplot->DivideSquare(ndists);
  for(auto ptbin : ROOT::TSeqI(0, ndists)){
    ratioplot->cd(ptbin+1);
    TLegend *leg(nullptr);
    (new ROOT6tools::TAxisFrame(Form("ratioframe_%s_R%02d_%s_%s", conf.fJetType.data(), int(conf.fR * 10.), conf.fTrigger.data(), conf.fObservable.data()), conf.fObservable.data(), "Re-folded/raw", distmin, distmax, 0.5, 1.5))->Draw("axis");
    if(!ptbin){
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.5, 0.89, 0.89);
      leg->Draw();
      (new ROOT6tools::TNDCLabel(0.15, 0.7, 0.5, 0.79, Form("%s, R=%.1f, %s", conf.fJetType.data(), conf.fR, conf.fTrigger.data())))->Draw();
    }
    auto ptmin = hraw->GetYaxis()->GetBinLowEdge(ptbin+1), ptmax = hraw->GetYaxis()->GetBinUpEdge(ptbin+1);
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.5, 0.89, Form("%.1f GeV/c < p_{t,j,d} < %.1f GeV/c", ptmin, ptmax)))->Draw();
    std::unique_ptr<TH1>rawslice(makePtProjection(*hraw, ptbin));
    auto iiter = 0;
    for(const auto &fold : hfold) {
      auto foldslice = makePtProjection(*fold.second, ptbin);
      foldslice->SetName(Form("RatioFoldRaw_%s", foldslice->GetName()));
      foldslice->Divide(rawslice.get());
      iterstyles[iiter++].SetStyle<decltype(*foldslice)>(*foldslice);
      foldslice->Draw("epsame");
      if(leg) leg->AddEntry(foldslice, Form("Iteration %d", fold.first), "lep");
    }
  }
  ratioplot->cd();
  ratioplot->Update();
}