#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/filesystem.C"

TH1 *makeJetSparseProjection(THnSparse *hsparse, int triggercluster, bool nefcut) {
  auto clusterbin = hsparse->GetAxis(5)->FindBin(triggercluster);
  std::cout << "Found cluster bin for projection " << clusterbin << std::endl;
  hsparse->GetAxis(5)->SetRange(clusterbin, clusterbin);
  if(nefcut){
    hsparse->GetAxis(4)->SetRangeUser(0., 0.98);
  }

  auto projected = hsparse->Projection(1);
  projected->SetDirectory(nullptr);

  // Unzoom
  hsparse->GetAxis(5)->UnZoom();
  if(nefcut) hsparse->GetAxis(4)->UnZoom();
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

std::string getDriname(TFile &reader, double radius, const std::string_view jettype, const std::string_view trigger){
  std::vector<std::string> tokens = {"JetSpectrum", std::string(jettype), Form("R%02d", int(radius*10.)), std::string(trigger)};
  std::string result;
  for(auto k : TRangeDynCast<TKey>(reader.GetListOfKeys())){
    std::string_view keyname(k->GetName());
    bool found(true);
    for(const auto &t : tokens) {
      if(keyname.find(t) == std::string::npos) {
        found = false;
        break;
      }
    }
    if(found) {
      result = std::string(keyname);
      break;
    }
  }
  return result;
}

std::array<TH1 *, 3> getNormalizedJetSpectrum(TFile &reader, double radius, const std::string_view jettype, const std::string_view trigger, int triggercluster){
  std::array<TH1 *, 3> result;
  std::stringstream eventsname, rawname, normalizedname;
  eventsname << "EventCount_" << jettype << "_R" << std::setw(2) << std::setfill('0') << int(radius * 10.) << "_" << trigger;
  auto dirname = getDriname(reader, radius, jettype, trigger);
  if(!dirname.length()) return result;
  reader.cd(dirname.data());
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());
  auto jetsparse = static_cast<THnSparse *>(histlist->FindObject("hJetTHnSparse"));

  auto norm = static_cast<TH1 *>(histlist->FindObject("hClusterCounter"));
  auto clusterbin = norm->GetXaxis()->FindBin(triggercluster);
  std::cout << "Found cluster bin for norm " << clusterbin << std::endl;
  auto eventcounter = new TH1F(eventsname.str().data(), "; trigger; number of events", 1, 0.5, 1.5);
  eventcounter->SetDirectory(nullptr);
  eventcounter->SetBinContent(1, norm->GetBinContent(clusterbin));
  result[0] = eventcounter;

  auto projected = makeJetSparseProjection(jetsparse, triggercluster, false);
  normalizedname << dirname << "_" << getClusterName(triggercluster);
  rawname << "Raw" << normalizedname.str(); 
  projected->SetName(rawname.str().data());
  projected->SetDirectory(nullptr);
  result[1] = projected;
  auto normalized = static_cast<TH1 *>(projected->Clone(normalizedname.str().data()));
  normalized->SetDirectory(nullptr);
  result[2] = normalized;
  normalized->Scale(1./eventcounter->GetBinContent(1));
  return result;
}

bool hasSpectrum(const TFile &reader, const std::string_view jettype, double radius, const std::string_view trigger){
  std::vector<std::string> tokens = {"JetSpectrum", std::string(jettype), std::string(trigger), Form("R%02d", int(radius*10.))};
  bool hasdir(false);
  for(auto k : TRangeDynCast<TKey>(reader.GetListOfKeys())){
    std::string_view keyname(k->GetName());
    bool found(true);
    for(const auto &t : tokens) {
      if(keyname.find(t) == std::string::npos) {
        found = false;
        break;
      }
    }
    if(found) {
      hasdir = true;
      break;
    }
  }
  return hasdir;
}

void extractJetSpectrum(const std::string_view inputfile = "AnalysisResults.root", int triggercluster = 0){
  const std::array<const std::string, 3> kJetTypes = {{"FullJets", "NeutralJets", "ChargedJets"}};
  const std::array<const std::string, 5> kTriggers = {{"INT7", "EG1", "EG2", "EJ1", "EJ2"}};
  
  auto dn = dirname(inputfile);
  std::stringstream outputfile;
  if(dn.length()) outputfile << dn << "/";
  outputfile << "JetSpectra_" << getClusterName(triggercluster);
  outputfile << ".root";
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
        auto spectra = getNormalizedJetSpectrum(*reader, radius, jt, trg, triggercluster);
        writer->cd(outdirname.str().data());
        for(auto spec : spectra) spec->Write();
      }
    }
  }
}