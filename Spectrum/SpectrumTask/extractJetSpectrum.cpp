#ifndef __CLING__
#include <array>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <THnSparse.h>
#include <TH1.h>
#include <TKey.h>
#include <TList.h>
#endif

#include "../../helpers/filesystem.C"

TH1 *makeJetSparseProjection(THnSparse *hsparse, int triggercluster, bool nefcut) {
  auto clusterbin = hsparse->GetAxis(4)->FindBin(triggercluster);
  hsparse->GetAxis(4)->SetRange(clusterbin);
  if(nefcut){
    hsparse->GetAxis(3)->SetRangeUser(0., 0.98);
  }

  auto projected = hsparse->Projection(0);
  projected->SetDirectory(nullptr);

  // Unzoom
  hsparse->GetAxis(4)->UnZoom();
  if(nefcut) hsparse->GetAxis(3)->UnZoom();
  return projected;
}

std::string getClusterName(int triggercluster) {
  std::string clustername;
  switch(triggercluster){
    case 0: clustername = "ANY"; break;
    case 1: clustername = "CENT"; break;
    case 2: clustername = "CENTNOTRD"; break;
    case 3: clustername = "CALO"; break;
    case 4: clustername = "CALOFAST"; break;
    case 5: clustername = "CENTBOTH"; break;
    case 6: clustername = "ONLYCENT"; break;
    case 7: clustername = "ONLYCENTNOTRD"; break;
    case 8: clustername = "CALOBOTH"; break;
    case 9: clustername = "ONLYCALO"; break;
    case 10: clustername = "ONLYCALOFAST"; break;
  };
  return clustername;
}

TH1 *getNormalizedJetSpectrum(TFile &reader, double radius, const std::string_view jettype, const std::string_view trigger, int triggercluster, bool doScale){
  std::stringstream dirname, normalizedname;
  dirname << "JetSpectrum_" << jettype << "_R" << std::setw(2) << std::setfill('0') << int(radius * 10.) << "_" << trigger;
  reader.cd(dirname.str().data());
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto jetsparse = static_cast<THnSparse *>(histlist->FindObject("hJetTHnSparse"));

  auto projected = makeJetSparseProjection(jetsparse, triggercluster, jettype == std::string_view("FullJets"));
  normalizedname << dirname.str() << "_" << getClusterName(triggercluster);
  projected->SetName(normalizedname.str().data());
  projected->SetDirectory(nullptr);
  if(doScale){
    auto norm = static_cast<TH1 *>(histlist->FindObject("hClusterCounter"));
    auto clusterbin = norm->GetXaxis()->FindBin(triggercluster);
    projected->Scale(1./norm->GetBinContent(clusterbin));
  }
  return projected;
}

bool hasSpectrum(const TFile &reader, const std::string_view jettype, double radius, const std::string_view trigger){
  std::stringstream dirname; 
  dirname << "JetSpectrum_" << jettype << "_R" << std::setw(2) << std::setfill('0') << int(radius * 10.) << "_" << trigger;
  bool found(false);
  for(auto k : *(reader.GetListOfKeys())) {
    std::string keyname = k->GetName();
    if(keyname == dirname.str()) {
      found = true;
      break;
    }
  }
  return found;
}

void extractJetSpectrum(const std::string_view inputfile = "AnalysisResults.root", int triggercluster = 0, bool doScale = true){
  const std::array<const std::string, 2> kJetTypes = {{"FullJets", "NeutralJets"}};
  const std::array<const std::string, 5> kTriggers = {{"INT7", "EG1", "EG2", "EJ1", "EJ2"}};
  
  auto dn = dirname(inputfile);
  std::stringstream outputfile;
  if(dn.length()) outputfile << dn << "/";
  outputfile << "JetSpectra_" << getClusterName(triggercluster) << ".root";
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ")),
                         writer(TFile::Open(outputfile.str().data(), "RECREATE"));

  for(const auto &jt : kJetTypes) {
    for(const auto &trg : kTriggers) {
      std::stringstream outdirname;
      outdirname << jt << "_" << trg;
      bool outputdirCreated(false);
      for(auto rad : ROOT::TSeqI(2, 6)){
        double radius = double(rad)/10.;
        if(!hasSpectrum(*reader, jt, radius, trg)) continue;
        if(!outputdirCreated) 
        {
          writer->mkdir(outdirname.str().data());
          outputdirCreated = true;
        }
        auto spec = getNormalizedJetSpectrum(*reader, radius, jt, trg, triggercluster, doScale);
        writer->cd(outdirname.str().data());
        spec->Write();
      }
    }
  }
}