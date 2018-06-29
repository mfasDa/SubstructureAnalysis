#ifndef __CLING__
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TFile.h>
#include <TH2.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TList.h>
#include <TROOT.h>

#include <TAxisFrame.h>
#include <TNDCLabel.h>
#include <TSavableCanvas.h>
#endif

int getClusterID(const std::string_view triggercluster){
  int clusterID = -1;
  if(triggercluster == "ANY") clusterID = 0;
  else if(triggercluster == "CENT") clusterID = 1;
  else if(triggercluster == "CENTNOTRD") clusterID = 2;
  else if(triggercluster == "CALO") clusterID = 3;
  else if(triggercluster == "CALOFAST") clusterID = 4;
  else if(triggercluster == "CENTBOTH") clusterID = 5;
  else if(triggercluster == "ONLYCENT") clusterID = 6;
  else if(triggercluster == "ONLYCENTNOTRD") clusterID = 7;
  else if(triggercluster == "CALOBOTH") clusterID = 8;
  else if(triggercluster == "ONLYCALO") clusterID = 9;
  else if(triggercluster == "ONLYCALOFAST") clusterID = 10;
  return clusterID;
}

TH2 *getEtaPhiDist(THnSparse *jetsparse, const std::string_view triggercluster){
  auto clusterID = getClusterID(triggercluster);
  jetsparse->GetAxis(4)->SetRange(clusterID + 1, clusterID + 1);
  jetsparse->GetAxis(0)->SetRangeUser(20., 100.);

  auto projected = jetsparse->Projection(2,1);
  projected->SetDirectory(nullptr);

  jetsparse->GetAxis(4)->UnZoom();
  jetsparse->GetAxis(0)->UnZoom();
  return projected;
}

std::vector<TH2 *> makeEtaPhiProjections(const std::string_view jettype, const std::string_view trigger, const std::string_view triggercluster, const std::string_view inputfile){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  std::vector<TH2 *> result;
  std::vector<std::thread> workers;
  std::mutex resultmutex, filemutex;
  for(auto radius : ROOT::TSeqI(2, 6)){
    workers.emplace_back(std::thread([&](int rad){
      THnSparse *jetsparse(nullptr);
      {
        std::unique_lock<std::mutex> filelock(filemutex);
        auto raddir = static_cast<TDirectoryFile *>(reader->Get(Form("JetSpectrum_%s_R%02d_%s", jettype.data(), rad, trigger.data())));
        auto histlist = static_cast<TKey *>(raddir->GetListOfKeys()->At(0))->ReadObject<TList>();
        jetsparse = static_cast<THnSparse *>(histlist->FindObject("hJetTHnSparse"));
      }
      auto etaphi = getEtaPhiDist(jetsparse, triggercluster);
      etaphi->SetName(Form("etaphi_%s_R%02d", jettype.data(), rad));
      std::unique_lock<std::mutex> resultlock(resultmutex);
      result.emplace_back(etaphi);
    }, radius));
  }
  for(auto &w : workers) w.join();

  return result;
}

void makeEtaPhiDists(const std::string_view jettype, const std::string_view trigger, const std::string_view triggercluster, const std::string_view inputfile){
  ROOT::EnableThreadSafety();

  auto plot = new ROOT6tools::TSavableCanvas(Form("etaphi%s%s%s", jettype.data(), trigger.data(), triggercluster.data()), Form(""), 1200, 1000);
  plot->Divide(2,2);

  auto dists = makeEtaPhiProjections(jettype, trigger, triggercluster, inputfile);
  for(auto r : ROOT::TSeqI(2, 6)){
    plot->cd(r-1);
    (new ROOT6tools::TAxisFrame(Form("etaphiframe_%s_%s_R%02d", jettype.data(), trigger.data(), r), "#eta", "#phi", -1., 1., 0., 6.5))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.25, 0.9, 0.75, 0.97, Form("%s, %s, R=%.1f, 20 GeV/c < p_{t,jet} < 100 GeV/c", jettype.data(), trigger.data(), double(r)/10.)))->Draw();
    (*std::find_if(dists.begin(), dists.end(), [&r](const TH2 *h){ return std::string_view(h->GetName()).find(Form("R%02d", r)) != std::string::npos; }))->Draw("colzsame");
  }
  plot->cd();
  plot->SaveCanvas(plot->GetName());
}