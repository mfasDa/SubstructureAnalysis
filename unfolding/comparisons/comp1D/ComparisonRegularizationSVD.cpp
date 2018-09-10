#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/filesystem.C"
#include "../../../helpers/graphics.C"
#include "../../../helpers/math.C"
#include "../../../helpers/root.C"
#include "../../../helpers/string.C"


TH1 *getIterationHist(TFile &reader, int reg){
  TH1 *result = nullptr;
  std::string regname = Form("regularization%d", reg);
  if(reader.GetListOfKeys()->FindObject(regname.data())){
    reader.cd(regname.data());
    std::string histname = Form("unfoldedReg%d", reg);
    result = static_cast<TH1 *>(gDirectory->Get(histname.data()));
    if(result){
      result->SetDirectory(nullptr);
      normalizeBinWidth(result);
    }
  }
  return result;
}

std::map<int, TH1 *> getUnfoldedHists(const std::string_view inputfile){
  std::map<int, TH1 *> result;
  std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
  // determine number of regulariztions
  int nreg(0);
  for(auto k: *reader->GetListOfKeys()) {
    if(contains(k->GetName(), "regularization")) nreg++;
  }
  for(auto ireg : ROOT::TSeqI(1, nreg+1)){
    if(auto h = getIterationHist(*reader, ireg)) result[ireg] = h;
  }
  return result;
}

std::string getFileTag(const std::string_view infile){
  const char *tagremove = "corrected1DSVD_";
  std::string filetag = basename(infile);
  filetag.erase(filetag.find(tagremove), strlen(tagremove));
  filetag.erase(filetag.find(".root"), 5);
  return filetag;
}

float getRadius(const std::string_view filetag) {
  return float(std::stoi(std::string(filetag.substr(1))))/10.;
}


void ComparisonRegularizationSVD(const std::string_view inputfile){
  auto regularizations = getUnfoldedHists(inputfile);
  auto tag = getFileTag(inputfile);
  auto radius = getRadius(tag);

  auto plot = new ROOT6tools::TSavableCanvas(Form("comparisonRegSVD_%s", tag.data()), "Conparison regularization SVD", 1200, 600);
  plot->Divide(2,1);
  
  plot->cd(1);
  gPad->SetLogy();
  (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "dN/dp_{t} ((GeV/c)^{-1})", 0., 250., 1e-10, 1e10))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.6, 0.89, 0.89);
  leg->Draw();
  (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.45, 0.22, Form("jets, R=%.1f,", radius)))->Draw();

  plot->cd(2);
  (new ROOT6tools::TAxisFrame("ratframe", "p_{t} (GeV/c)", "ratio to reg = 4", 0., 250., 0., 2.))->Draw("axis");

  std::array<Color_t, 10> colors = {{kRed, kBlue, kGreen, kViolet, kOrange, kTeal, kAzure, kGray, kMagenta, kCyan}};
  std::array<Style_t, 10> markers = {{24, 25, 26, 27, 28, 29, 30, 31, 32, 33}};

  for(auto ireg : ROOT::TSeqI(1, 11)){
    auto hist = regularizations[ireg];
    Style{colors[ireg-1], markers[ireg-1]}.SetStyle<TH1>(*hist);
    plot->cd(1);
    hist->Draw("epsame");
    leg->AddEntry(hist, Form("regularization = %d", ireg), "lep");
    
    if(ireg == 4) continue;
    plot->cd(2);
    auto ratio = histcopy(hist);
    ratio->SetDirectory(nullptr);
    ratio->SetName(Form("Ratio_%d_4", ireg));
    ratio->Divide(regularizations[4]);
    ratio->Draw("epsame");
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}