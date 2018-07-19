#ifndef __CLING__
#include <vector>
#endif

#include "../helpers/root.C"
#include "../helpers/graphics.C"

TH1 *getEfficiency(const std::string_view filename, const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL){
  std::cout << "Reading " << filename << std::endl;
  try {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::stringstream dirbuilder;
    dirbuilder << "ChargedParticleQA_" << tracktype.data();
    auto dir = static_cast<TDirectoryFile *>(reader->Get(dirbuilder.str().data()));
    auto histlist = static_cast<TList *>(static_cast<TKey *>(dir->GetListOfKeys()->At(0))->ReadObj());
    auto norm = static_cast<TH1 *>(histlist->FindObject(Form("hEventCount%s", trigger.data())));
    auto det = std::unique_ptr<THnSparse>(static_cast<THnSparse *>(histlist->FindObject(Form("hPtEtaPhiAll%s", trigger.data())))),
         part = std::unique_ptr<THnSparse>(static_cast<THnSparse *>(histlist->FindObject("hPtEtaPhiAllTrue")));
    // Look in front of EMCAL
    if(restrictEMCAL){
      det->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      det->GetAxis(2)->SetRangeUser(1.4, 3.1);
      part->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      part->GetAxis(2)->SetRangeUser(1.4, 3.1);
    } else {
      det->GetAxis(1)->SetRangeUser(-0.8, 0.8);
      part->GetAxis(1)->SetRangeUser(-0.8, 0.8);
    }
    auto projectedDet = det->Projection(0);
    std::unique_ptr<TH1> projectedPart(part->Projection(0));
    projectedDet->Sumw2();
    projectedPart->Sumw2();
    projectedDet->SetDirectory(nullptr);
    projectedPart->SetDirectory(nullptr);
    projectedDet->Divide(projectedDet, projectedPart.get(), 1., 1., "b");
    return projectedDet;
  } catch (...) {
    std::cout << "Failure ... " << std::endl;
    return nullptr;
  }
}

void compareTrackingEffJetJetMB(const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL){
  std::vector<int> runs  = {279312, 280998};

  auto plot = new ROOT6tools::TSavableCanvas(Form("effcompMBJJ_%s_%s_%s", tracktype.data(), trigger.data(), restrictEMCAL ? "EMCAL" : "Full"), "Comp eff MB, JJ", 1200, 1000);
  plot->Divide(2,2);

  Style mbstyle{kBlue, 24}, jjstyle{kRed, 25}, ratiostyle{kBlack, 20};

  for(auto ir : ROOT::TSeqI(0,2)){
    plot->cd(ir+1);
    (new ROOT6tools::TAxisFrame(Form("effcomp_%s_%s_%s_%d", tracktype.data(), trigger.data(), restrictEMCAL ? "EMCAL" : "Full", runs[ir]), "p_{t} (GeV/c)", "Efficiency", 0., 100., 0., 1.1))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.65, 0.89, Form("%s, %s, Run %d, %s acceptance", tracktype.data(), trigger.data(), runs[ir], restrictEMCAL ? "EMCAL" : "full")))->Draw();
    TLegend *leg = nullptr;
    if(!ir) {
      leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.22);
      leg->Draw();
    }
    auto jj = getEfficiency(Form("%d/AnalysisResults_JetJet.root", runs[ir]), tracktype, trigger, restrictEMCAL),
         mb =  getEfficiency(Form("%d/AnalysisResults_MB.root", runs[ir]), tracktype, trigger, restrictEMCAL);
    jj->SetName(Form("JetJet%d", runs[ir]));
    mb->SetName(Form("MinBias%d", runs[ir]));

    mbstyle.SetStyle<TH1>(*mb);
    jjstyle.SetStyle<TH1>(*jj);
    jj->Draw("epsame");
    mb->Draw("epsame");
    if(leg) {
      leg->AddEntry(mb, "Min. bias", "lep");
      leg->AddEntry(jj, "Jet-jet, LHC18f5", "lep"); 
    }

    plot->cd(ir+3);
    (new ROOT6tools::TAxisFrame(Form("effcomp_%s_%s_%s_%d", tracktype.data(), trigger.data(), restrictEMCAL ? "EMCAL" : "Full", runs[ir]), "p_{t} (GeV/c)", "MB/Jet-jet", 0., 100., 0.5, 1.5))->Draw("axis");
    auto ratio = histcopy(mb);
    ratio->SetName(Form("RatioMBJJ_%d", runs[ir]));
    ratio->Divide(jj);
    ratiostyle.SetStyle<TH1>(*ratio);
    ratio->Draw("epsame");
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}