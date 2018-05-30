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
#include <TString.h>
#include <TSystem.h>
#endif

#include "../../helpers/filesystem.C"
#include "../../helpers/string.C"

TH1 *makeJetSparseProjection(THnSparse *hsparse, int triggercluster, bool nefcut) {
  auto clusterbin = hsparse->GetAxis(4)->FindBin(triggercluster);
  hsparse->GetAxis(4)->SetRange(clusterbin, clusterbin);
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

TH1 *getNormalizedJetSpectrum(const std::string_view fname, double radius, const std::string_view jettype, const std::string_view trigger, int triggercluster, bool doScale){
  std::unique_ptr<TFile> reader(TFile::Open(fname.data(), "READ"));
  std::string dirname = reader->GetListOfKeys()->At(0)->GetName();
  std::stringstream normalizedname;
  reader->cd(dirname.data());
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto jetsparse = static_cast<THnSparse *>(histlist->FindObject("hJetTHnSparse"));

  auto projected = makeJetSparseProjection(jetsparse, triggercluster, jettype == std::string_view("FullJets"));
  normalizedname << "JetSpectrum_" << jettype << "_R" << std::setw(2) << std::setfill('0') << int(radius * 10.) << "_" << trigger << "_" << getClusterName(triggercluster);
  projected->SetName(normalizedname.str().data());
  projected->SetDirectory(nullptr);
  if(doScale){
    auto norm = static_cast<TH1 *>(histlist->FindObject("hClusterCounter"));
    auto clusterbin = norm->GetXaxis()->FindBin(triggercluster);
    projected->Scale(1./norm->GetBinContent(clusterbin));
  }
  return projected;
}

std::string matchSpectrum(const std::vector<std::string> &files, const std::string_view jettype, double radius, const std::string_view trigger){
  std::string result;
  auto data = std::find_if(files.begin(), files.end(), [jettype, radius, trigger](const std::string &fname) -> bool {
    TString fstr(fname);
    return fstr.Contains(jettype.data()) && fstr.Contains(trigger.data()) && fstr.Contains(Form("R%02d", int(radius * 10.))); 
  });
  if(data != files.end()) result = *data;
  return result;
}

std::vector<std::string> getListOfFiles(const std::string_view inputdir){
  std::string files = gSystem->GetFromPipe("ls -1 | grep JetSpectrum").Data();
  return tokenize(files, '\n');
}

void extractJetSpectrumFromSplit(const std::string_view inputdir = ".", int triggercluster = 0, bool doScale = true){
  const std::array<const std::string, 2> kJetTypes = {{"FullJets", "NeutralJets"}};
  const std::array<const std::string, 5> kTriggers = {{"INT7", "EG1", "EG2", "EJ1", "EJ2"}};

  auto specfiles = getListOfFiles(inputdir);
  
  std::stringstream outputfile;
  if(inputdir != std::string_view(".")) outputfile << inputdir << "/";
  outputfile << "JetSpectra_" << getClusterName(triggercluster);
  if(!doScale) outputfile << "_noscale" << ".root";
  std::unique_ptr<TFile> writer(TFile::Open(outputfile.str().data(), "RECREATE"));

  for(const auto &jt : kJetTypes) {
    for(const auto &trg : kTriggers) {
      std::stringstream outdirname;
      outdirname << jt << "_" << trg;
      bool outputdirCreated(false);
      for(auto rad : ROOT::TSeqI(2, 6)){
        double radius = double(rad)/10.;
        auto specfile = matchSpectrum(specfiles, jt, radius, trg);
        if(!specfile.length()) continue;
        std::cout << "Found spectra file " << specfile << std::endl;
        if(!outputdirCreated) 
        {
          writer->mkdir(outdirname.str().data());
          outputdirCreated = true;
        }
        auto spec = getNormalizedJetSpectrum(specfile, radius, jt, trg, triggercluster, doScale);
        writer->cd(outdirname.str().data());
        spec->Write();
      }
    }
  }
}