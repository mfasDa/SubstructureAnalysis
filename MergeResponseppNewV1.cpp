//////Full detector simulation//////////////////////
#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <tuple>
#include "TList.h"
#include "ROOT/TSeq.hxx"
#include <RStringView.h>
#include <TKey.h>
#include "TSystem.h"
#include "TFile.h"
#include "TDirectory.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TTree.h"
#include "TString.h"
#include "TProfile.h"
#include "TCanvas.h"
#endif

std::tuple<std::vector<int>, bool> GetPtHardBins(std::string_view inputdir){
  std::vector<int> pthardbins;
  bool usechilds(false);
  TString dirstring = gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data()));
  std::unique_ptr<TObjArray> dirs(dirstring.Tokenize("\n"));
  for(auto d : TRangeDynCast<TObjString>(dirs.get())){
    if(!d) continue;
    TString mydir = d->String();
    if(mydir.IsDigit()) {
      pthardbins.emplace_back(mydir.Atoi());
      usechilds = false;
    } else {
      if(mydir.Contains("child_")){
        mydir.ReplaceAll("child_", "");
        pthardbins.emplace_back(mydir.Atoi());
        usechilds = true;
      }
    }
  }
  return std::make_tuple(pthardbins, usechilds);
}

void MergeResponseppNewV1(std::string_view inputdir, std::string_view treename, std::string_view rootfile = "AnalysisResults.root"){
  auto respthardbins = GetPtHardBins(inputdir);
  auto pthardbins = std::get<0>(respthardbins);
  auto usechilds = std::get<1>(respthardbins);
  std::sort(pthardbins.begin(), pthardbins.end(), std::less<int>());
  std::cout << "Found " << pthardbins.size() << " pt-hard bins" << std::endl;
  TList weighted;
  std::vector<std::shared_ptr<TFile>> treefiles;

  Double_t pythiaweight(0.); // Variable needde for new branch
  TString dirname(treename);
  dirname.ReplaceAll("Tree", "");

  double Radius, EventWeight;
  int TriggerClusterIndex;
  double RhoPtRec, RhoPtSim, RhoMassRec, RhoMassSim, PtJetRec, EJetRec, EtaRec, PhiRec, AreaRec, MassRec, NEFRec;
  int NChargedRec, NNeutralRec;
  double PtJetSim, EJetSim, EtaSim, PhiSim, AreaSim, MassSim, NEFSim;
  int NChargedSim, NNeutralSim;
  double ZgMeasured,RgMeasured, MgMeasured, PtgMeasured, MugMeasured, DeltaRgMeasured;
  int NDroppedMeasured;
  double ZgTrue, RgTrue, MgTrue, PtgTrue, MugTrue, DeltaRgTrue;
  int NDroppedTrue;
  double OneSubjettinessMeasured, TwoSubjettinessMeasured, OneSubjettinessTrue, TwoSubjettinessTrue, AngularityMeasured, PtDMeasured, AngularityTrue, PtDTrue, PythiaWeight;
  int pthardbin;

  std::unique_ptr<TFile> output(TFile::Open(Form("%s_merged.root", treename.data()), "RECREATE"));
  auto outtree = new TTree("jetSubstructure", "Jet substructure tree");
  outtree->Branch("Radius", &Radius, "Radius/D");
  outtree->Branch("EventWeight", &EventWeight, "EventWeight/D");
  outtree->Branch("TriggerClusterIndex", &TriggerClusterIndex, "TriggerClusterIndex/I");
  outtree->Branch("RhoPtRec", &RhoPtRec, "RhoPtRec/D");
  outtree->Branch("RhoPtSim", &RhoPtSim, "RhoPtSim/D");
  outtree->Branch("RhoMassRec", &RhoMassRec, "RhoMassRec/D");
  outtree->Branch("RhoMassSim", &RhoMassSim, "RhoMassSim/D");
  outtree->Branch("PtJetRec", &PtJetRec, "PtJetRec/D");
  outtree->Branch("EJetRec", &EJetRec, "EJetRec/D");
  outtree->Branch("EtaRec", &EtaRec, "EtaRec/D");
  outtree->Branch("PhiRec", &PhiRec, "PhiRec/D");
  outtree->Branch("AreaRec", &AreaRec, "AreaRec/D");
  outtree->Branch("MassRec", &MassRec, "MassRec/D");
  outtree->Branch("NEFRec", &NEFRec, "NEFRec/D");
  outtree->Branch("NChargedRec", &NChargedRec, "NChargedRec/I");
  outtree->Branch("NNeutralSim", &NNeutralSim, "NNeutralSim/I");
  outtree->Branch("PtJetSim", &PtJetSim, "PtJetSim/D");
  outtree->Branch("EJetSim", &EJetSim, "EJetSim/D");
  outtree->Branch("EtaSim", &EtaSim, "EtaSim/D");
  outtree->Branch("PhiSim", &PhiSim, "PhiSim/D");
  outtree->Branch("AreaSim", &AreaSim, "AreaSim/D");
  outtree->Branch("MassSim", &MassSim, "MassSim/D");
  outtree->Branch("NEFSim", &NEFSim, "NEFSim/D");
  outtree->Branch("NChargedSim", &NChargedSim, "NChargedSim/I");
  outtree->Branch("NNeutralSim", &NNeutralSim, "NNeutralSim/I");
  outtree->Branch("ZgMeasured", &ZgMeasured, "ZgMeasured/D");
  outtree->Branch("RgMeasured", &RgMeasured, "RgMeasured/D");
  outtree->Branch("MgMeasured", &MgMeasured, "MgMeasured/D");
  outtree->Branch("PtgMeasured", &PtgMeasured, "PtgMeasured/D");
  outtree->Branch("MugMeasured", &MugMeasured, "MugMeasured/D");
  outtree->Branch("DeltaRgMeasured", &DeltaRgMeasured, "DeltaRgMeasured/D");
  outtree->Branch("NDroppedMeasured", &NDroppedMeasured, "NDroppedMeasured/I");
  outtree->Branch("ZgTrue", &ZgTrue, "ZgTrue/D");
  outtree->Branch("RgTrue", &RgTrue, "RgTrue/D");
  outtree->Branch("MgTrue", &MgTrue, "MgTrue/D");
  outtree->Branch("PtgTrue", &PtgTrue, "PtgTrue/D");
  outtree->Branch("MugTrue", &MugTrue, "MugTrue/D");
  outtree->Branch("DeltaRgTrue", &DeltaRgTrue, "DeltaTrue/D");
  outtree->Branch("NDroppedTrue", &NDroppedTrue, "NDroppedTrue/I");
  outtree->Branch("OneSubjettinessMeasured", &OneSubjettinessMeasured, "OneSubjettinessMeasured/D");
  outtree->Branch("TwoSubjettinessMeasured", &TwoSubjettinessMeasured, "TwoSubjettinessMeasured/D");
  outtree->Branch("OneSubjettinessTrue", &OneSubjettinessTrue, "OneSubjettinessTrue/D");
  outtree->Branch("TwoSubjettinessTrue", &TwoSubjettinessTrue, "TwoSubjettinessTrue/D");
  outtree->Branch("TwoSubjettinessTrue", &TwoSubjettinessTrue, "TwoSubjettinessTrue/D");
  outtree->Branch("AngularityMeasured", &AngularityMeasured, "AngularityMeasured/D");
  outtree->Branch("PtDMeasured", &PtDMeasured, "PtDMeasured/D");
  outtree->Branch("AngularityTrue", &AngularityTrue, "AngularityTrue/D");
  outtree->Branch("PtDTrue", &PtDTrue, "PtDTrue/D");
  outtree->Branch("PythiaWeight", &PythiaWeight, "PythiaWeight/D");
  outtree->Branch("PtHardBin", &pthardbin, "PtHardBin/I");

  for(auto b : pthardbins){
    // Read in weights
    TString inputfile = usechilds ? Form("%s/child_%d/%s", inputdir.data(), b, rootfile.data()) : Form("%s/%02d/%s", inputdir.data(), b, rootfile.data());
    std::shared_ptr<TFile> filereader(TFile::Open(inputfile, "READ")); 
    filereader->cd(dirname);
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    int databin = usechilds ? 1 : b+1;
    auto xsection = static_cast<TH1 *>(histos->FindObject("fHistXsection"))->GetBinContent(databin);
    auto nTrials = static_cast<TH1 *>(histos->FindObject("fHistTrials"))->GetBinContent(databin);
    auto nEvents = static_cast<TH1 *>(histos->FindObject("fHistEvents"))->GetBinContent(databin);
    PythiaWeight = xsection / nTrials;
    pthardbin = b;
    Printf("weight: %e, xsec: %f mb, Events / trials: %f bin: %d", PythiaWeight, xsection, nEvents/nTrials, b);

    // Read tree, reweight
    auto substructuretree = static_cast<TTree * >(filereader->Get(treename.data()));
    if(substructuretree) std::cout << "Found substructuretree " << substructuretree->GetName() << " in file " << inputfile << std::endl;
    substructuretree->SetBranchAddress("Radius", &Radius);
    substructuretree->SetBranchAddress("EventWeight", &EventWeight);
    substructuretree->SetBranchAddress("TriggerClusterIndex", &TriggerClusterIndex);
    substructuretree->SetBranchAddress("RhoPtRec", &RhoPtRec);
    substructuretree->SetBranchAddress("RhoPtSim", &RhoPtSim);
    substructuretree->SetBranchAddress("RhoMassRec", &RhoMassRec);
    substructuretree->SetBranchAddress("RhoMassSim", &RhoMassSim);
    substructuretree->SetBranchAddress("PtJetRec", &PtJetRec);
    substructuretree->SetBranchAddress("EJetRec", &EJetRec);
    substructuretree->SetBranchAddress("EtaRec", &EtaRec);
    substructuretree->SetBranchAddress("PhiRec", &PhiRec);
    substructuretree->SetBranchAddress("AreaRec", &AreaRec);
    substructuretree->SetBranchAddress("MassRec", &MassRec);
    substructuretree->SetBranchAddress("NEFRec", &NEFRec);
    substructuretree->SetBranchAddress("NChargedRec", &NChargedRec);
    substructuretree->SetBranchAddress("NNeutralSim", &NNeutralSim);
    substructuretree->SetBranchAddress("PtJetSim", &PtJetSim);
    substructuretree->SetBranchAddress("EJetSim", &EJetSim);
    substructuretree->SetBranchAddress("EtaSim", &EtaSim);
    substructuretree->SetBranchAddress("PhiSim", &PhiSim);
    substructuretree->SetBranchAddress("AreaSim", &AreaSim);
    substructuretree->SetBranchAddress("MassSim", &MassSim);
    substructuretree->SetBranchAddress("NEFSim", &NEFSim);
    substructuretree->SetBranchAddress("NChargedSim", &NChargedSim);
    substructuretree->SetBranchAddress("NNeutralSim", &NNeutralSim);
    substructuretree->SetBranchAddress("ZgMeasured", &ZgMeasured);
    substructuretree->SetBranchAddress("RgMeasured", &RgMeasured);
    substructuretree->SetBranchAddress("MgMeasured", &MgMeasured);
    substructuretree->SetBranchAddress("PtgMeasured", &PtgMeasured);
    substructuretree->SetBranchAddress("MugMeasured", &MugMeasured);
    substructuretree->SetBranchAddress("NDroppedMeasured", &NDroppedMeasured);
    substructuretree->SetBranchAddress("ZgTrue", &ZgTrue);
    substructuretree->SetBranchAddress("RgTrue", &RgTrue);
    substructuretree->SetBranchAddress("MgTrue", &MgTrue);
    substructuretree->SetBranchAddress("PtgTrue", &PtgTrue);
    substructuretree->SetBranchAddress("MugTrue", &MugTrue);
    substructuretree->SetBranchAddress("DeltaRgTrue", &DeltaRgTrue);
    substructuretree->SetBranchAddress("NDroppedTrue", &NDroppedTrue);
    substructuretree->SetBranchAddress("OneSubjettinessMeasured", &OneSubjettinessMeasured);
    substructuretree->SetBranchAddress("TwoSubjettinessMeasured", &TwoSubjettinessMeasured);
    substructuretree->SetBranchAddress("OneSubjettinessTrue", &OneSubjettinessTrue);
    substructuretree->SetBranchAddress("TwoSubjettinessTrue", &TwoSubjettinessTrue);
    substructuretree->SetBranchAddress("TwoSubjettinessTrue", &TwoSubjettinessTrue);
    substructuretree->SetBranchAddress("AngularityMeasured", &AngularityMeasured);
    substructuretree->SetBranchAddress("PtDMeasured", &PtDMeasured);
    substructuretree->SetBranchAddress("AngularityTrue", &AngularityTrue);
    substructuretree->SetBranchAddress("PtDTrue", &PtDTrue);
    for(auto en : ROOT::TSeqI(0, substructuretree->GetEntriesFast())){
      //printf("Doing entry %d\n", en);
      substructuretree->GetEntry(en);
      outtree->Fill();
    }
  }
}
