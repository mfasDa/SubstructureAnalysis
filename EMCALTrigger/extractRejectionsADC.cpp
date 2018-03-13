#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TList.h>
#endif

struct RejectionData {
  double threshold;
  double rejectionfactor;

};

std::ostream &operator<<(std::ostream &stream, const RejectionData &data) {
  stream << "Threshold: " << data.threshold << ", Rejection: " << data.rejectionfactor;
  return stream;
}

double getThreshold(TH1 *adctriggered) {
  return adctriggered->GetBinCenter(adctriggered->GetMaximumBin());
}

double getRejection(TH1 *adcminbias, Double_t threshold){
  return adcminbias->Integral() / adcminbias->Integral(adcminbias->GetXaxis()->FindBin(threshold), adcminbias->GetXaxis()->GetNbins());
}

bool IsGamma(const std::string_view trigger) {
  return trigger[1] == 'G';
}

bool IsJet(const std::string_view trigger) {
  return trigger[1] == 'J';
}

bool IsEmcal(const std::string_view trigger) {
  return trigger[0] == 'E';
}

bool IsDcal(const std::string_view trigger) {
  return trigger[0] == 'D';
}

std::map<std::string, RejectionData> extractTriggerRejections(const std::string_view filename, std::vector<std::string> listoftriggers) {
  TH1 *mbega(nullptr), *mbdga(nullptr),*mbeje(nullptr), *mbdje(nullptr) ;
  std::map<std::string, RejectionData> result;
  auto histreader = [](TFile &reader, const char *trigger) -> TH1 * {
    reader.cd(Form("MaxPatch%s", trigger));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>(); 
    return static_cast<TH1 *>(histlist->FindObject(Form("hPatchADCMax%s%sRecalc", IsEmcal(trigger) ? "E" : "D", IsJet(trigger) ? "JE" : "GA")));
  };
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  reader->cd("MaxPatchINT7");
  auto histlistMB = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
  mbega = static_cast<TH1 *>(histlistMB->FindObject("hPatchADCMaxEGARecalc"));
  mbeje = static_cast<TH1 *>(histlistMB->FindObject("hPatchADCMaxEJERecalc"));
  mbdga = static_cast<TH1 *>(histlistMB->FindObject("hPatchADCMaxDGARecalc"));
  mbdje = static_cast<TH1 *>(histlistMB->FindObject("hPatchADCMaxDJERecalc"));
  for(auto t : listoftriggers) {
    auto threshold = getThreshold(histreader(*reader, t.data()));
    auto hist = IsEmcal(t) ? (IsJet(t) ? mbeje : mbega) : (IsJet(t) ? mbdje : mbdga);
    result[t] = RejectionData{threshold, getRejection(hist, threshold)};
  }
  return result;
}

void extractRejectionsADC(const std::string_view filename){
  for(auto r : extractTriggerRejections(filename, {"EG1", "EG2", "EJ1", "EJ2", "DG1", "DG2", "DJ1", "DJ2"})) {
    std::cout << r.first << ": " << r.second << std::endl;
  }
}