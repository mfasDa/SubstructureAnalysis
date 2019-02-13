#include "../meta/stl.C"
#include "../meta/root.C"
#include "../helpers/math.C"

void extractTrackYield(const std::string_view filename, const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL){
  std::cout << "Reading " << filename << std::endl;
  try {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto dir = static_cast<TDirectoryFile *>(reader->Get(Form("ChargedParticleQA_%s_nocorr", tracktype.data())));
    auto histlist = static_cast<TList *>(static_cast<TKey *>(dir->GetListOfKeys()->At(0))->ReadObj());
    auto norm = static_cast<TH1 *>(histlist->FindObject(Form("hEventCount%s", trigger.data())));
    auto spec = std::unique_ptr<THnSparse>(static_cast<THnSparse *>(histlist->FindObject(Form("hPtEtaPhiAll%s", trigger.data()))));
    // Look in front of EMCAL
    if(restrictEMCAL){
      spec->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      spec->GetAxis(2)->SetRangeUser(1.4, 3.1);
    }
    auto projected = spec->Projection(0);
    projected->SetName("projected");
    projected->SetDirectory(nullptr);
    projected->Scale(1./norm->GetBinContent(1));
    normalizeBinWidth(projected);
    std::unique_ptr<TFile> writer(TFile::Open(Form("trackYield_%s_%s_%s.root", tracktype.data(), trigger.data(), (restrictEMCAL ? "EMCAL" : "full")), "RECREATE"));
    projected->Write();
  } catch (...) {
    std::cout << "Failure ... " << std::endl;
  }
}