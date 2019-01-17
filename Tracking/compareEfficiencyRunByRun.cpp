#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/filesystem.C"
#include "../helpers/graphics.C"
#include "../helpers/math.C"
#include "../helpers/string.C"

struct RunSpectrum{
  int fRun;
  TH1 *fSpectrum;

  bool operator==(const RunSpectrum &other) const { return fRun == other.fRun; }
  bool operator<(const RunSpectrum &other) const { return fRun < other.fRun; }
};

std::set<int> getListOfRuns(const std::string_view inputdir){
  std::set<int> result;
  for(auto d : tokenize(gSystem->GetFromPipe(Form("ls -1 %s", inputdir.data())).Data())){
    if(is_number(d)){
      if(!gSystem->AccessPathName(Form("%s/%s/AnalysisResults.root", inputdir.data(), d.data()))) result.insert(std::stoi(d));
    }
  }
  return result;
}

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
      det->GetAxis(1)->SetRangeUser(-0.6, 0.6);
      det->GetAxis(2)->SetRangeUser(1.4, 3.1);
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

ROOT6tools::TSavableCanvas *MakePlot(int index, const std::vector<int> &listofruns, const std::string_view inputdir, const std::string_view tracktype, const std::string_view trigger, bool restrictEMCAL, const std::map<double, TGraphErrors *> &trendgraphs){
  std::cout << "Restrincting to EMCAL acceptance: " << (restrictEMCAL ? "True" : "False") << std::endl;
  auto plot = new ROOT6tools::TSavableCanvas(Form("efficiencyComparison_%s_%s_%d", tracktype.data(), trigger.data(), index), Form("Efficiency comparison %s track (%s) %d", tracktype.data(), trigger.data(), index), 800, 600);
  plot->cd();

  (new ROOT6tools::TAxisFrame(Form("spectrumframe%s%s%d", tracktype.data(), trigger.data(), index), "p_{t} (GeV/c)", "Tracking efficiency", 0., 20., 0, 1.1))->Draw("axis");
  auto trklab =new ROOT6tools::TNDCLabel(0.15, 0.84, 0.5, 0.89, Form("Track type: %s", tracktype.data()));
  trklab->SetTextAlign(12);
  trklab->Draw();
  auto trglab = new ROOT6tools::TNDCLabel(0.15, 0.78, 0.4, 0.83, Form("Trigger: %s", trigger.data()));
  trglab->SetTextAlign(12);
  trglab->Draw();
  auto leg = new ROOT6tools::TDefaultLegend(0.2, 0.15, 0.65, 0.45);
  leg->SetNColumns(2);
  leg->Draw();

  // Parallelize reading spectra
  std::set<RunSpectrum> spectra;
  std::vector<std::thread> readingthreads;
  std::mutex specmutex;
  auto workertask = [&](int runnumber) {
    std::string infilename;
    infilename = Form("%s/%d/AnalysisResults.root", inputdir.data(), runnumber);
    auto spec = getEfficiency(infilename.data(), tracktype, trigger, restrictEMCAL);
    spec->SetName(Form("%s_%s_%d", trigger.data(), tracktype.data(), runnumber));
    //std::this_thread::sleep_for(std::chrono::duration<double, std::nano>(500));
    std::unique_lock<std::mutex> writelock(specmutex);    // write operation to std::set must be locked
    spectra.insert({runnumber, spec});
  };
  for(auto r : listofruns){
    readingthreads.emplace_back(workertask, r);
  }
  for(auto &t : readingthreads) t.join();

  std::array<Style, 10> styles = {{{kRed, 24}, {kBlue, 25}, {kGreen, 26}, {kViolet, 27}, {kOrange, 28}, {kMagenta, 29}, {kTeal, 30}, {kGray, 31}, {kAzure, 32}, {kBlack, 33}}};
  int ispec = 0;
  for(auto spectrum : spectra){
    styles[ispec++].SetStyle<TH1>(*spectrum.fSpectrum);
    spectrum.fSpectrum->Draw("epsame");
    leg->AddEntry(spectrum.fSpectrum, Form("%d", spectrum.fRun), "lep");

    // fill trending graphs
    for(auto t : trendgraphs){
      auto g = t.second;
      auto b = spectrum.fSpectrum->GetXaxis()->FindBin(t.first);
      auto n = g->GetN();
      g->SetPoint(n, spectrum.fRun, spectrum.fSpectrum->GetBinContent(b));
      g->SetPointError(n, 0., spectrum.fSpectrum->GetBinError(b));
    }
  }
  plot->Update();
  return plot;
}

void compareEfficiencyRunByRun(const std::string_view track_type, const std::string_view trigger, const std::string_view inputdir, bool restrictEMCAL){
  ROOT::EnableThreadSafety();
  std::map<double, TGraphErrors *> trending = {{0.5, new TGraphErrors}, {1., new TGraphErrors}, {2., new TGraphErrors}, {5., new TGraphErrors}};
  auto runs = getListOfRuns(inputdir);
  if(!runs.size()){
    std::cout << "No runs found in input dir " << inputdir << ", skipping ..." << std::endl;
    return; 
  } else {
    std::cout << "Found " << runs.size() << " runs in input dir " << inputdir << " ..." << std::endl;
  }

  int canvas = 0;
  std::vector<int> runsplot;
  for(auto r : runs) {
    runsplot.emplace_back(r);
    if(runsplot.size() == 10) {
      auto compplot = MakePlot(canvas++, runsplot, inputdir, track_type, trigger, restrictEMCAL, trending);
      compplot->SaveCanvas(compplot->GetName());
      runsplot.clear();
    }
  }
  if(runsplot.size()){
    auto compplot = MakePlot(canvas++, runsplot, inputdir, track_type, trigger, restrictEMCAL, trending);
    compplot->SaveCanvas(compplot->GetName());
  }

  // Draw trending
  auto trendplot = new ROOT6tools::TSavableCanvas(Form("efficiencyTrending_%s_%s", track_type.data(), trigger.data()), Form("Efficiency trending %s track %s", track_type.data(), trigger.data()), 1200, 1100);
  trendplot->Divide(2,2);
  Style trendstyle{kBlack, 20};

  std::set<double> ptref;
  int ipad = 1;
  for(auto en : trending) ptref.insert(en.first);   // no c++17, so sad ...
  for(auto pt : ptref ){
    trendplot->cd(ipad++);
    gPad->SetLeftMargin(0.18);
    gPad->SetRightMargin(0.1);
    auto trendgraph = trending.find(pt)->second;
    auto frame = new ROOT6tools::TAxisFrame(Form("trend%s%s_pt%d", track_type.data(), trigger.data(), int(pt * 10.)), "run", Form("Tracking efficiency|_{pt = %.1f GeV/c}", pt), *runs.begin(), *runs.rbegin(), 0., 1.);
    frame->GetXaxis()->SetNdivisions(305);
    frame->Draw("axis");
    if(ipad == 2){
      auto trklab =new ROOT6tools::TNDCLabel(0.2, 0.84, 0.55, 0.89, Form("Track type: %s", track_type.data()));
      trklab->SetTextAlign(12);
      trklab->Draw();
      auto trglab = new ROOT6tools::TNDCLabel(0.2, 0.78, 0.45, 0.83, Form("Trigger: %s", trigger.data()));
      trglab->SetTextAlign(12);
      trglab->Draw();
    }

    trendstyle.SetStyle<TGraphErrors>(*trendgraph);
    trendgraph->Draw("lepsame");
  }
  trendplot->SaveCanvas(trendplot->GetName());

  std::unique_ptr<TFile> writer(TFile::Open(Form("trendingEfficiency_%s_%s.root", track_type.data(), trigger.data()), "RECREATE"));
  writer->cd();
  for(auto t : trending){
    auto g = t.second;
    g->SetName(Form("trending_%03d", int(10. * t.first)));
    g->Write(g->GetName());
  }
}