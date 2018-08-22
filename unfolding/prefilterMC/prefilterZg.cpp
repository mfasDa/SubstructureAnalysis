#ifndef __CLING__
#include "RStringView.h"
#include "ROOT/RDataFrame.hxx"
#endif

#include "../../helpers/substructuretree.C"
#include "../../helpers/pthard.C"

void prefilterZg(const std::string_view inputfile) {
  auto tag = getFileTag(inputfile);
  auto jettreename = GetNameJetSubstructureTree(inputfile);
  ROOT::EnableImplicitMT(8);
  ROOT::RDataFrame df(jettreename, inputfile);

  auto filtered = df.Filter("NEFRec < 0.98").Filter([](Double_t pthardsim, Int_t pthardbin) { return !IsOutlier(pthardsim, pthardbin); }, {"PtJetSim", "PtHardBin"});
  filtered.Snapshot(jettreename, Form("JetSubstructureTree_%s_FilterZg.root", tag.data()), {"PtJetSim", "PtJetRec", "ZgMeasured", "ZgTrue", "PythiaWeight"});
}