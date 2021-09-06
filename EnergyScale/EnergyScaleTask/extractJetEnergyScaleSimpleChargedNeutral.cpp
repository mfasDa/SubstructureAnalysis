#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

const double kPtMin = 0.0,
             kPtMax = 200.;

double Median(const TH1 * h1) { 
  int n = h1->GetXaxis()->GetNbins();  
  std::vector<double>  x(n), y(n);
  for (int i = 0; i < n; i++) {
    x[i] = h1->GetBinCenter(i);
    y[i] = h1->GetBinContent(i);
  }
  // exclude underflow/overflows from bin content array y
  return TMath::Median(n, &x[0], &y[1]); 
}

std::array<double, 6> extractQuantiles(TH1 *slice){
  std::array<double, 6> result;
  result[0] = slice->GetMean();
  result[1] = slice->GetMeanError();

  result[2] = Median(slice);
  result[3] = 0;

  result[4] = slice->GetRMS();
  result[5] = slice->GetRMSError();
  return result;
}

std::array<TGraphErrors *, 9> getEnergyScaleForRadius(TFile &reader, const std::string_view jettype, int R, const std::string_view sysvar){
  std::array<TGraphErrors *, 9> result;
  std::stringstream dirnamebuilder;
  dirnamebuilder << "EnergyScaleResults_" <<  jettype << "_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
  if(sysvar.length()) dirnamebuilder << "_" << sysvar;
  reader.cd(dirnamebuilder.str().data());
  auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
  std::array<std::string, 3> consttype = {{"All", "Charged", "Neutral"}};
  for(std::size_t icharge = 0; icharge < consttype.size(); icharge++ ){
    auto chargename = consttype[icharge];
    TGraphErrors *mean = new TGraphErrors, *median = new TGraphErrors, *width = new TGraphErrors;
    mean->SetNameTitle(Form("mean%s_R%02d", chargename.data(), R), Form("Mean jet energy scale %s constituents (R=%.1f)", chargename.data(), double(R)/10.));
    median->SetNameTitle(Form("median%s_R%02d", chargename.data(), R), Form("Median jet energy scale %s constituents (R=%.1f)", chargename.data(), double(R)/10.));
    width->SetNameTitle(Form("width%s_R%02d", chargename.data(), R), Form("Jet energy resolution %s constituents (R=%.1f)", chargename.data(), double(R)/10.));
    std::string histname;
    if(chargename == "All") {
      histname = "hJetEnergyScale";
    } else {
      histname = Form("hJetEnergyScale%sVsFull", chargename.data());
    }

    auto h2d = static_cast<TH2 *>(histlist->FindObject(histname.data()));
    int current(0);
    // Make 5 GeV binning
    for(int ib = 0; ib < h2d->GetXaxis()->GetNbins(); ib += 5){
      auto projected = std::unique_ptr<TH1>(h2d->ProjectionY("py", ib+1, ib+5));
      auto quantiles = extractQuantiles(projected.get());
      double center = (h2d->GetXaxis()->GetBinLowEdge(ib+1) + h2d->GetXaxis()->GetBinUpEdge(ib+5))/2.,
             error = (h2d->GetXaxis()->GetBinUpEdge(ib+5) - h2d->GetXaxis()->GetBinLowEdge(ib+1))/2;
      mean->SetPoint(current, center, quantiles[0]);
      mean->SetPointError(current, error, quantiles[1]);
      median->SetPoint(current, center, quantiles[2]);
      median->SetPointError(current, error, quantiles[3]);
      width->SetPoint(current, center, quantiles[4]);
      width->SetPointError(current, error, quantiles[5]);
      current++;
    }
    result[3*icharge] = mean;
    result[3*icharge+1] = median;
    result[3*icharge+2] = width;
  }
  return result;
}


ROOT6tools::TSavableCanvas *makeOverviewPlot(std::map<int, std::array<TGraphErrors *, 9>> data, const std::string_view consttype, const std::string_view sysvar) {
  std::stringstream plotname, plottitle;
  plotname << "energyscaleplot_" << consttype;
  plottitle << "Energy scale (" << consttype;
  if(sysvar.length()) {
    plotname << "_" << sysvar;
    plottitle << ", " << sysvar;
  }
  plottitle << ")";
  auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), plottitle.str().data(), 1200,  600);
  plot->Divide(3,1);

  plot->cd(1);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto meanframe = new TH1F("meanframe", "; p_{t,part} (GeV/c); <(p_{t,det} - p_{t,part})/p_{t,part}>", (int)kPtMax, kPtMin, kPtMax);
  meanframe->SetDirectory(nullptr);
  meanframe->SetStats(false);
  meanframe->GetYaxis()->SetRangeUser(-0.5, 0.5);
  meanframe->Draw("axis");

  auto leg = new TLegend(0.65, 0.56, 0.89, 0.89);
  InitWidget(*leg);
  leg->Draw();

  auto chargelabel = new TPaveText(0.4, 0.5, 0.89, 0.55, "NDC");
  InitWidget(*chargelabel);
  chargelabel->AddText(Form("%s constituents", consttype.data()));
  chargelabel->Draw();

  plot->cd(2);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto medianframe = new TH1F("medianframe", "; p_{t,part} (GeV/c); median((p_{t,det} - p_{t,part})/p_{t,part})", (int)kPtMax, kPtMin, kPtMax);
  medianframe->SetDirectory(nullptr);
  medianframe->SetStats(false);
  medianframe->GetYaxis()->SetRangeUser(-0.5, 0.5);
  medianframe->Draw("axis");;

  plot->cd(3);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto widthframe = new TH1F("widthframe", "; p_{t,part} (GeV/c); #sigma((p_{t,det} - p_{t,part})/p_{t,part})", (int)kPtMax, kPtMin, kPtMax);
  widthframe->SetDirectory(nullptr);
  widthframe->SetStats(false);
  widthframe->GetYaxis()->SetRangeUser(0., .4);
  widthframe->Draw("axis");

  std::size_t icharge = -1;
  if(consttype == "All") icharge = 0;
  else if(consttype == "Charged") icharge = 1;
  else if(consttype == "Neutral") icharge = 2;
  std::size_t offset = icharge * 3;

  std::map<int, Style> radii = {{2, {kRed, 24}}, {3, {kBlue, 25}}, {4, {kGreen, 26}}, {5, {kViolet, 27}}, {6, {kOrange, 28}}};
  for(const auto R : radii){
    auto enscale = data.find(R.first);
    if(enscale != data.end()){
      for(auto i : ROOT::TSeqI(0,3)){
        plot->cd(i+1);
        auto graph = enscale->second[offset + i];
        R.second.SetStyle(*graph);
        graph->Draw("epsame");
        if(i==0) leg->AddEntry(graph, Form("R=%.1f", double(R.first)/10.), "lep");  
      }
    }
  }
  plot->cd();
  plot->Update();
  return plot;
}

ROOT6tools::TSavableCanvas *makePlotChargeComparison(std::map<int, std::array<TGraphErrors *, 9>> data, int radius, const std::string_view sysvar) {
  std::stringstream plotname, plottitle;
  plotname << "energyscaleplot_R" << std::setw(2) << std::setfill('0') << radius;
  plottitle << "Energy scale (R=" << std::setprecision(1) << (double(radius)/10.);
  if(sysvar.length()) {
    plotname << "_" << sysvar;
    plottitle << ", " << sysvar;
  }
  plottitle << ")";
  auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), plottitle.str().data(), 1200,  600);
  plot->Divide(3,1);

  plot->cd(1);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto meanframe = new TH1F("meanframe", "; p_{t,part} (GeV/c); <(p_{t,det} - p_{t,part})/p_{t,part}>", (int)kPtMax, kPtMin, kPtMax);
  meanframe->SetDirectory(nullptr);
  meanframe->SetStats(false);
  meanframe->GetYaxis()->SetRangeUser(-0.5, 0.5);
  meanframe->Draw("axis");

  auto leg = new TLegend(0.4, 0.65, 0.89, 0.89);
  InitWidget(*leg);
  leg->Draw();

  auto chargelabel = new TPaveText(0.7, 0.5, 0.89, 0.55, "NDC");
  InitWidget(*chargelabel);
  chargelabel->AddText(Form("R=%.1f", double(radius)/10.));
  chargelabel->Draw();

  plot->cd(2);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto medianframe = new TH1F("medianframe", "; p_{t,part} (GeV/c); median((p_{t,det} - p_{t,part})/p_{t,part})",(int)kPtMax, kPtMin, kPtMax);
  medianframe->SetDirectory(nullptr);
  medianframe->SetStats(false);
  medianframe->GetYaxis()->SetRangeUser(-0.5, 0.5);
  medianframe->Draw("axis");;

  plot->cd(3);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto widthframe = new TH1F("widthframe", "; p_{t,part} (GeV/c); #sigma((p_{t,det} - p_{t,part})/p_{t,part})", (int)kPtMax, kPtMin, kPtMax);
  widthframe->SetDirectory(nullptr);
  widthframe->SetStats(false);
  widthframe->GetYaxis()->SetRangeUser(0., .4);
  widthframe->Draw("axis");

  auto enscale = data.find(radius);
  if(enscale != data.end()){
    std::map<std::string, Style> chargetypes = {{"All", {kRed, 24}}, {"Charged", {kBlue, 25}}, {"Neutral", {kGreen, 26}}};
  
    for(auto i : ROOT::TSeqI(0,3)){
      plot->cd(i+1);
      for(const auto charge : chargetypes){
        std::size_t icharge = -1;
        if(charge.first == "All") {
          icharge = 0;
        } else if(charge.first == "Charged") {
          icharge = 1;
        } else if(charge.first == "Neutral") {
          icharge = 2;
        }
        auto graph = enscale->second[icharge * 3 + i];
        charge.second.SetStyle(*graph);
        graph->Draw("epsame");
        if(i==0) leg->AddEntry(graph, Form("%s constituents", charge.first.data()), "lep");  
      }
    }
  }
  plot->cd();
  plot->Update();
  return plot;
}

void extractJetEnergyScaleSimpleChargedNeutral(const std::string_view filename = "AnalysisResults.root", const std::string_view jettype = "FullJet", const std::string_view sysvar = "tc200"){
  std::stringstream outfilename;
  outfilename << "EnergyScaleResults";
  if(sysvar.length()) {
    outfilename << "_" << sysvar;
  }
  outfilename << ".root";

  std::map<int, std::array<TGraphErrors *, 9>> data;
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
  for(const auto R : ROOT::TSeqI(2, 7))
    data[R] = getEnergyScaleForRadius(*reader, jettype, R, sysvar);
  
  std::array<std::string, 3> consttypes = {{"All", "Charged", "Neutral"}};
  for(auto chargetype : consttypes) {
    auto plot = makeOverviewPlot(data, chargetype, sysvar);
    plot->SaveCanvas(plot->GetName());
  }
  for(const auto R : ROOT::TSeqI(2, 7)) {
    auto plot = makePlotChargeComparison(data, R, sysvar);
    plot->SaveCanvas(plot->GetName());
  }

  // Write output
  auto writer = std::unique_ptr<TFile>(TFile::Open(outfilename.str().data(), "RECREATE"));
  for(auto [radius, histos] : data) {
    std::string rstring = Form("R%02d", radius);
    writer->mkdir(rstring.data());
    writer->cd(rstring.data());
    for(auto hist : histos) hist->Write();
  }
}