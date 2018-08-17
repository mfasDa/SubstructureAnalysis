#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"

struct unfoldconfig {
  std::string fJetType;
  double fR;
  std::string fTrigger;
  std::string fObservable;
};

unfoldconfig extractFileTokens(const std::string_view filename){
  auto tokens = tokenize(std::string(filename.substr(0, filename.find(".root"))), '_');
  return {tokens[1], double(std::stoi(tokens[2].substr(1,2)))/10., tokens[3], tokens[5]};
}

void makePlotRawCounts(const std::string_view inputfile){
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  auto conf = extractFileTokens(inputfile);

  auto hraw = static_cast<TH2 *>(reader->Get("hraw"));
  hraw->SetDirectory(nullptr);
  hraw->SetStats(false);
  hraw->SetTitle("");
  hraw->SetXTitle("z_{g}");
  hraw->SetYTitle("p_{t} (GeV/c)");

  auto plot = new ROOT6tools::TSavableCanvas(Form("RawCounts_%s_R%02d_%s_%s", conf.fJetType.data(), int(conf.fR * 10.), conf.fTrigger.data(), conf.fObservable.data()),
                                             Form("Raw counts %s %s, R=%.1f, %s", conf.fObservable.data(), conf.fJetType.data(), conf.fR, conf.fTrigger.data()),
                                             800, 600);
  plot->cd();
  hraw->Draw("TEXT");
  (new ROOT6tools::TNDCLabel(0.15, 0.9, 0.5, 0.95, Form("%s, R=%.1f, %s", conf.fJetType.data(), conf.fR, conf.fTrigger.data())))->Draw();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}