#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

void makeTRDtrackingEff(const std::string_view filename){
  std::map<int, TH1 *> efficiencies;
  std::map<int, std::string> titles = {{0, "global, with TRD update"}, 
                                       {1, "comp. a, with TRD update"}, 
                                       {2, "comp. b, with TRD update"},
                                       {3, "Non-ITSrefit, with TRD update"},
                                       {4, "global, without TRD update"},
                                       {5, "comp. a, without TRD update"}, 
                                       {6, "comp. b, without TRD update"},
                                       {7, "Non-ITSrefit, without TRD update"},
                                       };
  {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "AnalysisResults.root"));
    auto data = static_cast<TList *>(reader->Get("AliEmcalTrackingQATask_histos"));
    auto hrec = static_cast<THnSparse *>(data->Get("fParticlesMatched")),
         htrue = static_cast<THnSparse *>(data->Get("fParticlesPhysPrim"));
    TH1 *hSpecTrue = htrue->Projection(0);
    for(auto tt :: ROOT::TSeqI(0, 8)) {
      hrec->GetAxis(6)->SetRange(tt+1, tt+1);
      auto heff = hrec->Projection(0);
      heff->SetDirectory(nullptr);
      heff->Divide(heff, hSpecTrue, 1., 1., 'b');
      efficiencies[tt] = heff;
    }
  }

  auto plot = new ROOT6tools::TSavableCanvas("trackingEff", "Tracking Efficiency", 800, 600);
  (new ROOT6tools::TAxisFrame("effframe", "; p_{t,gen} (GeV/c); #epsilon_{trk}", 0., 200., 0., 1.2))->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.75, 0.89, 0.89);
  leg->SetNColumns(2);
  leg->Draw();
  int marker = 24;
  std::array<Color_t, 8> colors = {{kRed, kBlue, kGreen, kOrange, kViolet, kTeal, kMagenta, kGray}};
  for(auto tt : ROOT::TSeqI(0, 8)){
    auto effhist = efficiencies[tt];
    Style{colors[tt], marker++}.SetStyle<TH1>(*effhist);
    effhist->Draw("epsame");
    leg->AddEntry(effhist, titles[tt].data(), "lep");
  }
  plot->cd();
  plot->Update();
}