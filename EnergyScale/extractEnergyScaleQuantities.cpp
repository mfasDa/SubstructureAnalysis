#ifndef __CLING__
#include <memory>
#include <vector>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TH2.h>
#endif

struct point {
  double fX, fMean, fMedian, fResolution, fEX, fEMean, fEMedian, fEResolution;

  bool operator==(const point &other) const {
    return fX < other.fX;
  }

  bool operator<(const point &other) const {
    return fX < other.fX;
  }
};

point make_point(TH1 &ptslice, double x, double ex) {
  double vmedian, quantile(0.5);
  ptslice.GetQuantiles(1, &vmedian, &quantile);
  return {x, ptslice.GetMean(), vmedian, ptslice.GetRMS(), ex, ptslice.GetMeanError(), 0, ptslice.GetRMSError()};
}

std::vector<point> GetData(TH2 *ptdiff2d) {
  std::vector<point> result;
  for(auto b : ROOT::TSeqI(1, ptdiff2d->GetXaxis()->GetNbins()+1)) {
    result.emplace_back(make_point(*std::unique_ptr<TH1>(ptdiff2d->ProjectionY("py", b, b)), ptdiff2d->GetXaxis()->GetBinCenter(b), ptdiff2d->GetXaxis()->GetBinWidth(b)/2.));
  }
  std::sort(result.begin(), result.end(), std::less<point>());
  return result;
}

void extractEnergyScaleQuantities(double radius) {
  auto reader = std::unique_ptr<TFile>(TFile::Open(Form("EnergyScale_R%02d.root", int(radius*10.)), "READ"));
  auto data = GetData(static_cast<TH2 *>(reader->Get("ptdiff")));

  auto mean = new TGraphErrors, median = new TGraphErrors, resolution = new TGraphErrors;
  int np = 0;
  for(auto p : data){
    mean->SetPoint(np, p.fX, p.fMean);
    mean->SetPointError(np, p.fEX, p.fEMean);
    median->SetPoint(np, p.fX, p.fMedian);
    median->SetPointError(np, p.fEX, p.fEMedian);
    resolution->SetPoint(np, p.fX, p.fResolution);
    resolution->SetPointError(np, p.fEX, p.fEResolution);
    np++;
  }

  auto writer = std::unique_ptr<TFile>(TFile::Open(Form("JESQuantities_R%02d.root", int(radius*10.)), "RECREATE"));
  writer->cd();
  mean->Write("mean");
  median->Write("median");
  resolution->Write("resolution");
}