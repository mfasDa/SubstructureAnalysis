#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/substructuretree.C"
#include "../binnings/binningZg.C"

std::vector<double> getLinearBinning(int nbins, double xmin, double xmax) {
  std::vector<double> bins;
  bins.push_back(xmin);
  auto db = (xmax - xmin)/double(nbins);
  for(auto i = 0; i < nbins; i++) bins.push_back(xmin + i * db);
  return bins;
}

void fillMCRawCountsBin(const std::string_view filename){
  ROOT::EnableImplicitMT(8);

  auto ptbinning = getPtBinningPart("EJ1"), pthardbinning = getLinearBinning(21, -0.5, 20.5);
  ROOT::RDataFrame df(GetNameJetSubstructureTree(filename), filename);
  auto selected = df.Filter("ZgTrue >= 0.20 && ZgTrue < 0.25");
  auto weighted = selected.Histo2D({"hweighted", "weighted; pt-hard bin; p_{t,j} (GeV/c)", static_cast<int>(pthardbinning.size()-1), pthardbinning.data(), static_cast<int>(ptbinning.size()-1), ptbinning.data()}, "PtHardBin", "PtJetSim", "PythiaWeight");
  auto unweighted = selected.Histo2D({"hunweighted", "weighted; pt-hard bin; p_{t,j} (GeV/c)", static_cast<int>(pthardbinning.size()-1), pthardbinning.data(), static_cast<int>(ptbinning.size()-1), ptbinning.data()}, "PtHardBin", "PtJetSim");

  auto tag = getFileTag(filename);
  auto plot = new ROOT6tools::TSavableCanvas(Form("spectraPthard_%s", tag.data()), "spectra in pt-hard bins", 1200, 600);
  plot->Divide(2,1);
  plot->cd(1);
  gPad->SetLogy();
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  (new ROOT6tools::TAxisFrame("frameweighted", "p_{t,jet} (GeV/c)", "d#sigma/dp_{t} (mb/(GeV/c))", 0., 200., 1e-12, 1))->Draw("axis");
  auto leg = new ROOT6tools::TDefaultLegend(0.19, 0.65, 0.94, 0.89);
  leg->SetNColumns(4);
  leg->Draw();
  plot->cd(2);
  gPad->SetLogy();
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  (new ROOT6tools::TAxisFrame("frameweighted", "p_{t,jet} (GeV/c)", "dN/dp_{t} ((GeV/c)^-1)", 0., 200., 5e-1, 1e7))->Draw("axis");
  
  std::array<Color_t, 11> colors = {{kBlack, kRed, kBlue, kGreen, kTeal, kViolet, kOrange, kGray, kCyan, kMagenta, kAzure}};
  std::array<Style_t, 8> markers = {{24, 25, 26, 27, 28, 29, 30, 31}};
  
  int icol = 0, imark = 0;
  for(auto ib : ROOT::TSeqI(1, 21)){
    auto bin = weighted->GetXaxis()->FindBin(ib);
    auto pweight = weighted->ProjectionY(Form("weighted%d", ib), bin, bin),
         pabscounts = unweighted->ProjectionY(Form("abscounts%d", ib), bin, bin);
    normalizeBinWidth(pweight);
    auto binstyle = Style{colors[icol], markers[imark]};
    binstyle.SetStyle<TH1>(*pweight);
    binstyle.SetStyle<TH1>(*pabscounts);

    plot->cd(1);
    pweight->Draw("epsame");
    leg->AddEntry(pweight, Form("bin %d", ib), "lep");
    
    plot->cd(2);
    pabscounts->Draw("epsame");

    icol++; imark++;
    if(icol == 11) icol = 0;
    if(imark == 8) imark = 0;
  }
  plot->cd();
  plot->Update();
  plot->SaveCanvas(plot->GetName());
}