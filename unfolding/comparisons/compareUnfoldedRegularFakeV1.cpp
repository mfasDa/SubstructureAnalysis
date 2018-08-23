#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include "ROOT/TSeq.hxx"
#include "RStringView.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TKey.h"

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"
#include "../../helpers/substructuretree.C"

const double fakeweight = 30.;

void convertFakeToRegular(TH1 *fakehist) {
  fakehist->SetBinContent(1, fakehist->GetBinContent(1) / fakeweight);
  fakehist->SetBinError(1, fakehist->GetBinError(1) / fakeweight);
}

std::map<int, TH2 *> readIterations(const std::string_view inputfile){
  std::map<int, TH2 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  reader->cd();
  auto hraw = static_cast<TH2 *>(reader->Get("hraw"));
  hraw->SetDirectory(nullptr);
  result[-1] = hraw;
  for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())){
    if(contains(k->GetName(), "iteration")) {
      reader->cd(k->GetName());
      auto numiter = std::stoi(std::string(k->GetName()).substr(9));
      auto histos = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
      auto unfolded = (*std::find_if(histos.begin(), histos.end(), [](const TKey *k) { return contains(k->GetName(), "unfolded_iter"); } ))->ReadObject<TH2>();
      unfolded->SetDirectory(nullptr);
      result[numiter] = unfolded;
    }
  }
  return result;
}

void compareUnfoldedRegularFakeV1(const std::string_view regularfile, const std::string_view fakefile){
  auto histos_regular = readIterations(regularfile),
       histos_fake = readIterations(fakefile);

  // determine plot range
  auto rawhist = histos_regular[-1];
  double ptmin = rawhist->GetYaxis()->GetBinLowEdge(1),
         ptmax = rawhist->GetYaxis()->GetBinUpEdge(rawhist->GetYaxis()->GetNbins());
  std::cout << "plotting unfolded within " << ptmin << " and " << ptmax << " GeV/c ..." << std::endl;
  auto iter1 = histos_regular[1];
  auto firstbin = iter1->GetYaxis()->FindBin(ptmin + 1.),
       lastbin  = iter1->GetYaxis()->FindBin(ptmax - 1.),
       nbins = lastbin - firstbin + 1;

  auto tag = getFileTag(regularfile);
  auto plotspec = new ROOT6tools::TSavableCanvas(Form("compRegularFakeV1_%s", tag.data()), "Comparison normal fake", 1200, 1000);
  plotspec->DivideSquare(nbins);

  auto plotratio = new ROOT6tools::TSavableCanvas(Form("ratioRegularFakeV1_%s", tag.data()), "Ratio normal fake", 1200, 1000);
  plotratio->DivideSquare(nbins);

  const std::map<int, Color_t> colors = {{1, kRed}, {4, kBlue}, {10, kGreen}, {25, kMagenta}};
  const std::map<std::string, Style_t> markers = {{"regular", 24}, {"fake", 25}};

  int currentpad = 1;
  for(auto ptbin : ROOT::TSeqI(firstbin, lastbin+1)){
    plotspec->cd(currentpad);
    (new ROOT6tools::TAxisFrame(Form("specframe_%d", ptbin), "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.6, 0., 0.5))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.7, 0.22, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", iter1->GetYaxis()->GetBinLowEdge(ptbin), iter1->GetYaxis()->GetBinUpEdge(ptbin))))->Draw();
    TLegend *leg(nullptr), *legratio(nullptr);
    if(currentpad == 1) {
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.5, 0.89, 0.89);
      leg->Draw();
    }
    plotratio->cd(currentpad);
    (new ROOT6tools::TAxisFrame(Form("ratioframe_%d", ptbin), "z_{g}", "Fake/Regular", 0., 0.6, 0.5, 1.5))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.7, 0.22, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", iter1->GetYaxis()->GetBinLowEdge(ptbin), iter1->GetYaxis()->GetBinUpEdge(ptbin))))->Draw();
    if(currentpad == 1){
      legratio = new ROOT6tools::TDefaultLegend(0.15, 0.7, 0.89, 0.89);
      legratio->SetNColumns(2);
      legratio->Draw();
    }
    for(auto iter : colors) {
      auto h2regular = histos_regular.find(iter.first)->second,
           h2fake = histos_fake.find(iter.first)->second;
      auto hproreg = h2regular->ProjectionX(Form("regular_iter%d_ptbin%d", iter.first, ptbin), ptbin, ptbin);
      hproreg->SetDirectory(nullptr);
      hproreg->Scale(1./hproreg->Integral());
      auto hprofake = h2fake->ProjectionX(Form("fake_iter%d_ptbin%d", iter.first, ptbin), ptbin, ptbin);
      convertFakeToRegular(hprofake);
      hprofake->SetDirectory(nullptr);
      hprofake->Scale(1./hprofake->Integral());

      Style{iter.second, markers.find("regular")->second}.SetStyle<TH1>(*hproreg);
      Style{iter.second, markers.find("fake")->second}.SetStyle<TH1>(*hprofake);

      auto ratio = histcopy(hprofake);
      ratio->SetName(Form("ratio__%s__%s", hprofake->GetName(), hproreg->GetName()));
      ratio->SetDirectory(nullptr);
      ratio->Divide(hproreg);
      Style{iter.second, 25}.SetStyle<TH1>(*ratio);

      plotspec->cd(currentpad);
      hproreg->Draw("epsame");
      hprofake->Draw("epsame");
      if(leg){
        leg->AddEntry(hproreg, Form("Normal, iter = %d", iter.first), "lep");
        leg->AddEntry(hprofake, Form("Fake, iter = %d", iter.first), "lep");
      }

      plotratio->cd(currentpad);
      ratio->Draw("epsame");
      if(legratio) legratio->AddEntry(ratio, Form("iter = %d", iter.first), "lep");
    }
    currentpad++;
  }
  plotspec->cd();
  plotspec->Update();
  plotspec->SaveCanvas(plotspec->GetName());
  plotratio->cd();
  plotratio->Update();
  plotratio->SaveCanvas(plotratio->GetName());
}
