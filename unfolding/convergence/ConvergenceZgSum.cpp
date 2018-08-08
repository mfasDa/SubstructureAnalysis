#ifndef __CLING__
#include <map>
#include <memory.h>
#include <mutex>
#include <thread>
#include <sstream>
#include <RStringView.h>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH2.h>

#include "TAxisFrame.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

TGraphErrors *getConvergence(std::map<int, TH2 *> &data, int zgbin) {
  TGraphErrors *result = new TGraphErrors(35);
  std::mutex fillmutex;
  for(auto iter : ROOT::TSeqI(1,36)){
    std::thread filler([&](int i) {
      std::unique_ptr<TH1> slice(data[i]->ProjectionX());
      slice->Scale(1./slice->Integral());
      std::lock_guard<std::mutex> filllock(fillmutex);
      result->SetPoint(i-1, i, slice->GetBinContent(zgbin));
      result->SetPointError(i-1, 0, slice->GetBinError(zgbin));
    }, iter);
    filler.join();
  }
  return result;
}

std::map<int, TH2 *> readIterations(const std::string_view infile){
  std::map<int, TH2 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(infile.data(), "READ"));
  for(auto iter : ROOT::TSeqI(1, 36)){
    reader->cd(Form("iteration%d", iter));
    auto h2d = static_cast<TH2 *>(gDirectory->Get(Form("zg_unfolded_iter%d", iter)));
    h2d->SetDirectory(nullptr);
    result[iter] = h2d;
  }
  return result;
}

std::pair<double, double> getValueRange(const TGraphErrors *graph) {
  double min = DBL_MAX, max = DBL_MIN;
  for(auto p : ROOT::TSeqI(0., graph->GetN())){
    auto pmin = graph->GetY()[p] - graph->GetEY()[p], pmax = graph->GetY()[p] + graph->GetEY()[p];
    if(pmin < min) min = pmin;
    if(pmax > max) max = pmax;
  } 
  return {min, max};
}

void ConvergenceZgSum(const std::string_view infile) {
  auto data = readIterations(infile);
  int nbinszg = data[1]->GetXaxis()->GetNbins();
  auto filebase = infile.substr(0, infile.find(".root"));

  std::stringstream canvasname;
  canvasname << filebase << "_convsum";

  auto plot = new ROOT6tools::TSavableCanvas(canvasname.str().data(), Form("zg-convergence (pt-integrated)"), 1200, 1000);
  plot->DivideSquare(nbinszg);
  for(auto zgbin : ROOT::TSeqI(1, nbinszg+1)){
    plot->cd(zgbin);
    gPad->SetLeftMargin(0.18);
    gPad->SetRightMargin(0.02);
    auto conv = getConvergence(data, zgbin);
    auto limits = getValueRange(conv);
    if(TMath::Abs(limits.first - limits.second) < DBL_EPSILON) limits = {limits.first - 0.01, limits.second + 0.01};
    (new ROOT6tools::TAxisFrame(Form("iterframe_zg_%d", zgbin), "Number of iterations", "1/N_{jet} dN/dz_{g}", 0., 40., limits.first * 0.9, limits.second * 1.1))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.2, 0.77, 0.97, 0.89, Form("%.2f <= zg < %.2f", data[1]->GetXaxis()->GetBinLowEdge(zgbin), data[1]->GetXaxis()->GetBinUpEdge(zgbin))))->Draw();
    conv->SetMarkerColor(kBlack);
    conv->SetLineColor(kBlack);
    conv->SetMarkerStyle(20);
    conv->Draw("lepsame");
  }
  plot->cd();
  plot->SaveCanvas(plot->GetName());
}