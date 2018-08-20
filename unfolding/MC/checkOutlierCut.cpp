#ifndef __CLING__ 
#include "ROOT/RDataFrame.hxx"

#include "TAxisFrame.h"
#include "TDefaultLegend.h"
#include "TNDCLabel.h"
#include "TSavableCanvas.h"
#endif

#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/pthard.C"
#include "../../helpers/root.C"
#include "../../helpers/substructuretree.C"
#include "../binnings/binningZg.C"

void checkOutlierCut(const std::string_view mcfile){
  ROOT::EnableImplicitMT(8);

  auto ptbinning = getMinBiasPtBinningPart();
  ROOT::RDataFrame df(GetNameJetSubstructureTree(mcfile), mcfile);

  // Apply outlier cuts
  auto dataCutNoCut = df.Filter("NEFRec < 0.98");
  auto dataCut2 = dataCutNoCut.Filter([](double ptsim, int pthardbin) { return !IsOutlier(ptsim, pthardbin, 2.); }, {"PtJetSim", "PtHardBin"});
  auto dataCut3 = dataCutNoCut.Filter([](double ptsim, int pthardbin) { return !IsOutlier(ptsim, pthardbin, 3.); }, {"PtJetSim", "PtHardBin"});
  auto dataCut5 = dataCutNoCut.Filter([](double ptsim, int pthardbin) { return !IsOutlier(ptsim, pthardbin, 5.); }, {"PtJetSim", "PtHardBin"});
  auto dataCut10 = dataCutNoCut.Filter([](double ptsim, int pthardbin) { return !IsOutlier(ptsim, pthardbin, 10.); }, {"PtJetSim", "PtHardBin"});

  auto histNoCut = dataCutNoCut.Histo1D({"specTrueNoOutlierCut", "true spectrum no outlier cut", static_cast<int>(ptbinning.size()-1), ptbinning.data()}, "PtJetSim", "PythiaWeight"),
       histCut2 = dataCut2.Histo1D({"specTrueOutlierCut2", "true spectrum outlier cut 2", static_cast<int>(ptbinning.size()-1), ptbinning.data()}, "PtJetSim", "PythiaWeight"),
       histCut3 = dataCut3.Histo1D({"specTrueOutlierCut3", "true spectrum outlier cut 3", static_cast<int>(ptbinning.size()-1), ptbinning.data()}, "PtJetSim", "PythiaWeight"),
       histCut5 = dataCut5.Histo1D({"specTrueOutlierCut5", "true spectrum outlier cut 5", static_cast<int>(ptbinning.size()-1), ptbinning.data()}, "PtJetSim", "PythiaWeight"),
       histCut10 = dataCut10.Histo1D({"specTrueOutlierCut10", "true spectrum outlier cut 10", static_cast<int>(ptbinning.size()-1), ptbinning.data()}, "PtJetSim", "PythiaWeight");

  normalizeBinWidth(histNoCut.GetPtr());
  normalizeBinWidth(histCut2.GetPtr());
  normalizeBinWidth(histCut3.GetPtr());
  normalizeBinWidth(histCut5.GetPtr());
  normalizeBinWidth(histCut10.GetPtr());

  auto tag = getFileTag(mcfile);
  auto plot = new ROOT6tools::TSavableCanvas(Form("outliercut_truespectrum_%s", tag.data()), "Outlier cut comparison", 1200, 600);
  plot->Divide(2,1); 
  
  plot->cd(1);
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "d#sigma/dp_{t}", 0., 200., 1e-8, 1e-2))->Draw("axis");
  (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.5, 0.22, tag.data()))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.7, 0.65, 0.89, 0.89);
  leg->Draw();

  auto specNoCut = histcopy(histNoCut.GetPtr());
  specNoCut->SetDirectory(nullptr);
  Style{kBlack, 20}.SetStyle<TH1>(*specNoCut);
  specNoCut->Draw("epsame");
  leg->AddEntry(specNoCut, "No cut", "lep");

  auto specCut2 = histcopy(histCut2.GetPtr());
  specCut2->SetDirectory(nullptr);
  Style{kRed, 24}.SetStyle<TH1>(*specCut2);
  specCut2->Draw("epsame");
  leg->AddEntry(specCut2, "Outlier cut 2", "lep");

  auto specCut3 = histcopy(histCut3.GetPtr());
  specCut3->SetDirectory(nullptr);
  Style{kBlue, 25}.SetStyle<TH1>(*specCut3);
  specCut3->Draw("epsame");
  leg->AddEntry(specCut3, "Outlier cut 3", "lep");

  auto specCut5 = histcopy(histCut5.GetPtr());
  specCut5->SetDirectory(nullptr);
  Style{kGreen, 26}.SetStyle<TH1>(*specCut5);
  specCut5->Draw("epsame");
  leg->AddEntry(specCut5, "Outlier cut 5", "lep");

  auto specCut10 = histcopy(histCut10.GetPtr());
  specCut10->SetDirectory(nullptr);
  Style{kViolet, 27}.SetStyle<TH1>(*specCut10);
  specCut10->Draw("epsame");
  leg->AddEntry(specCut10, "Outlier cut 10", "lep");

  plot->cd(2);
  (new ROOT6tools::TAxisFrame("ratframe", "p_{t} (GeV/c)", "ratio to no cut", 0., 200., 0.8, 1.2))->Draw("axis");
  auto rat2 = histcopy(specCut2);
  rat2->SetDirectory(nullptr);
  rat2->Divide(rat2, specNoCut, 1., 1., "b");
  rat2->Draw("epsame");

  auto rat3 = histcopy(specCut3);
  rat3->SetDirectory(nullptr);
  rat3->Divide(rat3, specNoCut, 1., 1., "b");
  rat3->Draw("epsame");

  auto rat5 = histcopy(specCut5);
  rat5->SetDirectory(nullptr);
  rat5->Divide(rat5, specNoCut, 1., 1., "b");
  rat5->Draw("epsame");

  auto rat10 = histcopy(specCut10);
  rat10->SetDirectory(nullptr);
  rat10->Divide(rat10, specNoCut, 1., 1., "b");
  rat10->Draw("epsame");

  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}