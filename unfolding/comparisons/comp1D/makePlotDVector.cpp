#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/string.C"

struct JetDef {
  std::string fJetType;
  double fJetRadius;
  std::string fTrigger;
};

std::string getFileTag(const std::string_view infile){
  std::string filetag = basename(infile);
  const std::string tagremove = contains(filetag, "Svd") ? "unfoldedEnergySvd_" : "unfoldedEnergyBayes_";
  filetag.erase(filetag.find(tagremove), tagremove.length());
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

JetDef getJetType(const std::string_view filetag) {
  auto tokens = tokenize(std::string(filetag), '_');
  return {tokens[0], double(std::stoi(tokens[1].substr(1)))/10., tokens[2]};
}

void makePlotDVector(const std::string_view filename) {
  std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
  reader->cd("regularization1");
  TH1 *dvector = static_cast<TH1 *>(gDirectory->Get("dvectorReg1"));
  dvector->SetDirectory(nullptr);  
  dvector->SetXTitle("d-vector");
  dvector->SetYTitle("Entries");
  dvector->SetTitle("");
  dvector->SetStats(false);

  auto tag = getFileTag(filename);
  auto jd = getJetType(tag);
  auto plot = new ROOT6tools::TSavableCanvas(Form("dvector_%s", tag.data()), "Response matrix", 800, 600);
  plot->cd();
  plot->SetLogy();
  dvector->Draw("b");
  (new ROOT6tools::TNDCLabel(0.35, 0.8, 0.75, 0.87, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data())))->Draw();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}