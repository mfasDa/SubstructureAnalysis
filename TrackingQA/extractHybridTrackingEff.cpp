#ifndef __CLING__
#include <RStringView.h>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TList.h>
#endif

template<typename t>
std::unique_ptr<t> make_unique(t *ptr) { 
  return std::unique_ptr<t>(ptr); 
}

TH1 *Project(THnSparse *inputhist, std::string_view outputname){
  auto result = inputhist->Projection(0);
  result->SetName(outputname.data());
  return result;
}

void CorrectBinWidth(TH1 *hist) {
  for(auto b : ROOT::TSeqI(1, hist->GetXaxis()->GetNbins()+1)){
    auto bw = hist->GetXaxis()->GetBinWidth(b);
    hist->SetBinContent(b, hist->GetBinContent(b)/bw);
    hist->SetBinError(b, 0); //hist->GetBinError(b)/bw); // Was not running under sumw2
  }
}

void extractHybridTrackingEff(std::string_view inputfile = "AnalysisResults.root", std::string_view hybriddef = "hybrid"){
  auto inputreader = make_unique<TFile>(TFile::Open(inputfile.data(), "READ"));
  inputreader->cd(Form("ChargedParticleQA_%s", hybriddef.data()));
  auto histlist = static_cast<TList *>(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObj());

  auto htruth = static_cast<THnSparse *>(histlist->FindObject("hPtEtaPhiAllTrue")),
       hrec = static_cast<THnSparse *>(histlist->FindObject("hPtEtaPhiAllMB"));

  auto projecttruth = Project(htruth, "spectrumTrue"),
       projectrec = Project(hrec, "spectrumRec");
  projecttruth->SetDirectory(nullptr);
  projectrec->SetDirectory(nullptr);
  //projecttruth->Sumw2();
  //projectrec->Sumw2();

  auto efficiency = static_cast<TH1 *>(projectrec->Clone("efficiency"));
  efficiency->SetDirectory(nullptr);
  efficiency->Divide(projectrec, projecttruth, 1, 1, "b");

  CorrectBinWidth(projecttruth);
  CorrectBinWidth(projectrec);

  auto outputwriter = make_unique<TFile>(TFile::Open(Form("TrackingEfficiency_%s.root", hybriddef.data()), "RECREATE"));
  outputwriter->cd();
  projecttruth->Write();
  projectrec->Write();
  efficiency->Write();
}