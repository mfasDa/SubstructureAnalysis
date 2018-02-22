#ifndef __CLING__
#include <sstream>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TProfile.h>
#endif

#include "../helpers/graphics.C"

TProfile *GetPtResolution(TFile &data, int tracktype, int cuttype, const char *trigger) {
  std::stringstream dirname;
  dirname << "PWG4_HighPtTrackQACent10TrackType" << tracktype << "Cuts" << cuttype << trigger;
  std::cout << "Reading " << dirname.str().data() << std::endl;
  data.cd(dirname.str().data());
  auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();

  auto ptres = static_cast<TProfile*>(histlist->FindObject("fProfPtPtSigma1Pt"));
  ptres->SetDirectory(nullptr);
  return ptres;
}

void DrawType(TFile &datareader, TFile &mcreader, int tracktype, int cuttype, bool dolegend) {
  auto frame = new TH1F(Form("frame_%d_%d", tracktype, cuttype), "; p_{t} (GeV/c); p_{t} * #sigma(1/p_{t})", 200, 0., 200.);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->GetYaxis()->SetRangeUser(0., 1.);
  frame->Draw("axis");

  std::map<std::string, Style> styles = {{"MC INT7", {kBlack, 20}}, {"INT7", {kBlack, 24}}, {"EJ1", {kRed, 25}}, {"EJ2", {kBlue, 25}}};
  
  TLegend *leg(nullptr);
  if(dolegend){
    leg = new TLegend(0.15, 0.65, 0.4, 0.89);
    InitWidget<TLegend>(*leg);
    leg->Draw();
  }
  auto tracklabel = new TPaveText(0.65, 0.12, 0.89, 0.18, "NDC");
  InitWidget<TPaveText>(*tracklabel);
  tracklabel->AddText(Form("Track type %d, cuts %d", tracktype, cuttype));
  tracklabel->Draw();
  
  for(auto trg : styles) {
    bool mc(false);
    auto trgname = trg.first;
    if(trgname.find("MC") != std::string::npos){
      mc = true;
      trgname = "INT7";
    } 
    if(trgname.find("INT7") != std::string::npos) trgname = "kINT7";
    auto res = GetPtResolution(mc ? mcreader : datareader,tracktype, cuttype, trgname.data());
    trg.second.SetStyle<decltype(*res)>(*res);
    res->Draw("epsame");
    if(leg) leg->AddEntry(res, trg.first.data(), "lep");
  }
  gPad->Update();
}

void makeComparisonPtResolution(const char *datafile, const char *mcfile){
  auto plot = new TCanvas("ptresplot", "pt-resolution", 1200, 500);
  plot->Divide(3,1);

  auto datareader = std::unique_ptr<TFile>(TFile::Open(datafile, "READ"));
  auto mcreader = std::unique_ptr<TFile>(TFile::Open(mcfile, "READ"));
  std::vector<std::pair<int,int>> tracktypes = {{0,0}, {0,5}, {7,5}};
  auto pad = 1;
  for(auto t : tracktypes) {
    plot->cd(pad++);
    DrawType(*datareader, *mcreader, t.first, t.second, pad == 2);
  }
  plot->cd();
  plot->Update();
}