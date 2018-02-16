#ifndef __CLING__
#include <array>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <ROOT/TSeq.hxx>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <THnSparse.h>
#include <TKey.h>
#include <TLegend.h>
#include <TList.h>
#include <TPaveText.h>
#include <TROOT.h>
#endif

#include "../helpers/graphics.C"

constexpr double kVerySmall = 1e-5;
const std::map<std::string, Style> kStylesData = {{"EJ1", {kRed, 21}}, {"EJ2", {kBlue, 22}}, {"INT7", {kBlack, 20}}},
                                   kStylesMC = {{"EJ1", {kRed, 24}}, {"EJ2", {kBlue, 25}}, {"INT7", {kBlack, 26}}};


struct THnSparseHandler{
  enum Observable_t {
    kUndef = -1,
    kPtCh = 4,
    kZCh = 5,
    kENe = 5,
    kZNe = 6
  };
  std::shared_ptr<THnSparse> jetSparse;
  std::shared_ptr<THnSparse> constituentSparse;

  TH1 *GetProjectedDist(double ptmin, double ptmax, Observable_t observable) {
    auto globptmin = constituentSparse->GetAxis(0)->GetFirst(), globptmax = constituentSparse->GetAxis(0)->GetLast(),
         binptmin = constituentSparse->GetAxis(0)->FindBin(ptmin + kVerySmall), binptmax = constituentSparse->GetAxis(0)->FindBin( ptmax - kVerySmall);
    constituentSparse->GetAxis(0)->SetRange(binptmin, binptmax);
    auto result = constituentSparse->Projection(observable);
    auto norm  = std::unique_ptr<TH1>(jetSparse->Projection(0));
    result->SetDirectory(nullptr);
    result->Scale(1./norm->Integral(binptmin, binptmax));
    constituentSparse->GetAxis(0)->SetRange(globptmin, globptmax);
    return result;
  }
};

class Frame {
public:
  Frame() = default;
  Frame(std::string_view xtitle, std::string_view ytitle, std::array<double, 4> boundaries, bool logframe = false):
    fXtitle(xtitle),
    fYtitle(ytitle),
    fBoundaries(boundaries),
    fLogframe(logframe)
  {
  }
  ~Frame() noexcept = default;

  TH1 *MakeFrame(const char *name) const {
    auto result = new TH1F(name, "", 100, fBoundaries[0], fBoundaries[1]);
    result->SetDirectory(0);
    result->SetStats(0);
    result->SetXTitle(fXtitle.data());
    result->SetYTitle(fYtitle.data());
    result->GetYaxis()->SetRangeUser(fBoundaries[2], fBoundaries[3]);
    return result;
  }

  void SetLogY(bool logframe) { fLogframe = logframe; }
  bool IsLogY() const { return fLogframe; }

private:
  std::string             fXtitle;
  std::string             fYtitle;
  std::array<double, 4>   fBoundaries;
  Bool_t                  fLogframe;
};

class GenericPanel : public TCanvas {
public:
  GenericPanel() = default;
  GenericPanel(const char *name, const char *title, double radius, THnSparseHandler::Observable_t observable):
    TCanvas(name, title, 1000, 800),
    fRadius(radius),
    fObservable(observable),
    fAxisFrame(),
    fDataMB(),
    fDataTrg(),
    fMCMB()
  {
    Divide(3,3);
  }
  void SetDataMB(const std::map<std::string, THnSparseHandler> &datamb) { fDataMB = datamb; }
  void SetDataTrg(const std::map<std::string, THnSparseHandler> &datatrg) { fDataTrg = datatrg; }
  void SetMCMB(const std::map<std::string, THnSparseHandler> &mcmb) { fMCMB = mcmb; }
  void SetFrame(const Frame &frame) { fAxisFrame = frame; }

  void CreatePanel() {
    auto panel = 1;
    TLegend *leg(nullptr);
    for(auto ptmin : ROOT::TSeqI(20, 200, 20)){
      auto ptmax = ptmin + 20;
      cd(panel++);
      if(fAxisFrame.IsLogY()) gPad->SetLogy();
      fAxisFrame.MakeFrame(Form("framePtChR%02d_%d_%d", int(fRadius*10.), ptmin, ptmax))->Draw("axis");

      if(panel ==2 ) {
        leg = new TLegend(0.65, 0.7, 0.89, 0.89);
        InitWidget<TLegend>(*leg);
        leg->Draw();

        auto jetslabel = new TPaveText(0.55, 0.15, 0.89, 0.22, "NDC");
        InitWidget<TPaveText>(*jetslabel);
        jetslabel->AddText(Form("Full jets, R=%.1f", fRadius));
        jetslabel->Draw();
      }

      auto ptlabel = new TPaveText(0.15, 0.8, 0.65, 0.89, "NDC");
      InitWidget<TPaveText>(*ptlabel);
      ptlabel->AddText(Form("%.1f GeV/c < p_{t,jet} < %.1f GeV/c", double(ptmin), double(ptmax)));
      ptlabel->Draw();

      fProcessFunction(fDataTrg, fMCMB, double(ptmin), double(ptmax), "Data", panel == 2 ? leg : nullptr);
      fProcessFunction(fDataMB, fMCMB, double(ptmin), double(ptmax), "Data", panel == 2 ? leg : nullptr);
      fProcessFunction(fMCMB, fMCMB, double(ptmin), double(ptmax), "MC", panel == 2 ? leg : nullptr);
      gPad->Update();
    }
    cd();
    Update();
  }

protected:
  double                                      fRadius;
  THnSparseHandler::Observable_t              fObservable;
  Frame                                       fAxisFrame;
  std::map<std::string, THnSparseHandler>     fDataMB;
  std::map<std::string, THnSparseHandler>     fDataTrg;
  std::map<std::string, THnSparseHandler>     fMCMB;
  std::function<void (std::map<std::string, THnSparseHandler> &, std::map<std::string, THnSparseHandler> &, double, double, const char *, TLegend *)> fProcessFunction;
};

class SpectrumPanel : public GenericPanel {
public:
  SpectrumPanel() = default;
  SpectrumPanel(const char *name, const char *title, double radius, THnSparseHandler::Observable_t observable):
    GenericPanel(name, title, radius, observable)
  {
    fProcessFunction = [this](std::map<std::string, THnSparseHandler> &h, std::map<std::string, THnSparseHandler> &r, double min, double max, const char *tag, TLegend *leg){
      this->PlotSpectra(h,r,min,max,tag,leg);
    };
  }
  virtual ~SpectrumPanel() noexcept = default;

  void PlotSpectra(std::map<std::string, THnSparseHandler >&handler, std::map<std::string, THnSparseHandler> &ref, double ptmin, double ptmax, const char *tag, TLegend *leg) {
    const auto &stylehandler = (strcmp(tag, "Data") == 0) ? kStylesData : kStylesMC;
    for(auto h : handler) {
      auto spec  = h.second.GetProjectedDist(ptmin, ptmax, fObservable);
      spec->SetName(Form("spec_%d_%d_R%02f_%s_%s", int(ptmin), int(ptmax), fRadius, h.first.data(), tag));
      stylehandler.find(h.first)->second.SetStyle<TH1>(*spec);
      spec->Draw("epsame");
      if(leg) leg->AddEntry(spec, Form("%s, %s", tag, h.first.data()), "lep");
    }
  }
};

class RatioPanel : public GenericPanel {
public:
  RatioPanel() = default;
  RatioPanel(const char *name, const char *title, double radius, THnSparseHandler::Observable_t observable):
    GenericPanel(name, title, radius, observable)
  {
    fProcessFunction = [this](std::map<std::string, THnSparseHandler> &h, std::map<std::string, THnSparseHandler> &r, double min, double max, const char *tag, TLegend *leg){
      this->PlotRatios(h,r,min,max,tag,leg);
    };
  }
  virtual ~RatioPanel() noexcept = default;

  void PlotRatios(std::map<std::string, THnSparseHandler >&handler, std::map<std::string, THnSparseHandler> &ref, double ptmin, double ptmax, const char *tag, TLegend *leg) {
    if(!strcmp(tag, "MC")) return;

    auto refspectrum = std::unique_ptr<TH1>(ref.find("INT7")->second.GetProjectedDist(ptmin, ptmax, fObservable));
    for(auto h : handler) {
      auto spec  = h.second.GetProjectedDist(ptmin, ptmax, fObservable);
      spec->SetName(Form("spec_%d_%d_R%02f_%s_%s", int(ptmin), int(ptmax), fRadius, h.first.data(), tag));
      spec->Divide(refspectrum.get());
      kStylesData.find(h.first)->second.SetStyle<TH1>(*spec);
      spec->Draw("epsame");
      if(leg) leg->AddEntry(spec, Form("%s, %s", tag, h.first.data()), "lep");
    }
  }
};


std::map<std::string, THnSparseHandler> ExtractDataFromFile(const char *filename, double radius, bool mb, bool charged) {
  auto triggers = std::array<std::string, 3>{{"INT7", "EJ1", "EJ2"}};
  std::stringstream rstring;
  rstring << "R" << std::setw(2) << std::setfill('0') << int(radius * 10);
  std::map<std::string, THnSparseHandler> result;
  auto reader = std::unique_ptr<TFile>(TFile::Open(filename, "READ"));
  auto keylist = reader->GetListOfKeys();
  const char *sparsename = charged ? "hChargedConstituents" : "hNeutralConstituents";
  for(auto t : triggers){
    if(mb) {
      if(t.find("INT7") == std::string::npos) continue;
    } else {
      if(t.find("INT7") != std::string::npos) continue;
    }
    auto trgkey = std::string("JetConstituentQA_") + t; 
    if(!keylist->FindObject(trgkey.data())) continue;
    reader->cd(trgkey.data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    gROOT->cd();
    THnSparse *jetsparse(nullptr), *constituentsparse(nullptr);
    for(auto h : TRangeDynCast<THnSparse>(histlist)){
      if(!h) continue;
      std::string histname = h->GetName();
      if(histname.find("hJetCounter") != std::string::npos && histname.find(rstring.str()) != std::string::npos) {
        jetsparse = h;
      } else if(histname.find(sparsename) != std::string::npos && histname.find(rstring.str()) != std::string::npos){
        constituentsparse = h;
      }
    }
    // Apply cut on the neutral energy
    jetsparse->GetAxis(1)->SetRange(jetsparse->GetAxis(1)->FindBin(0.02  + kVerySmall), jetsparse->GetAxis(1)->FindBin(0.98 - kVerySmall));
    constituentsparse->GetAxis(1)->SetRange(jetsparse->GetAxis(1)->FindBin(0.02 + kVerySmall), jetsparse->GetAxis(1)->FindBin(0.98 - kVerySmall));
    result.insert(std::pair<std::string, THnSparseHandler>(t, {std::shared_ptr<THnSparse>(jetsparse), std::shared_ptr<THnSparse>(constituentsparse)}));
  }
  return result;
}

void MakeRatioPtCh(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius) {
  auto plot = new RatioPanel(Form("RatioSIMptchR%02d", int(radius*10.)),Form("Ratio pt_ch for jets with R=%.1f", radius),radius, THnSparseHandler::kPtCh);
  plot->SetFrame(Frame("p_{t,ch} (GeV/c)","Ratio to INT7_MC",{{0., 200., 0.5, 1.5}}, false));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void MakeRatioZCh(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius) {
  auto plot = new RatioPanel(Form("RatioSIMzchR%02d", int(radius*10.)),Form("Ratio z_ch for jets with R=%.1f", radius),radius, THnSparseHandler::kZCh);
  plot->SetFrame(Frame("z_{ch} (GeV/c)","Ratio to INT7_MC",{{0., 1., 0.5, 1.5}}, false));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void MakeComparisonPtCh(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius){
  auto plot = new SpectrumPanel(Form("ptchR%02d", int(radius*10.)),Form("pt_ch for jets with R=%.1f", radius),radius, THnSparseHandler::kPtCh);
  plot->SetFrame(Frame("p_{t,ch} (GeV/c)"," 1/N_{jet} dN/dp_{t,ch} ((GeV/c)^{-1})",{{0., 200., 1e-6, 1.}}, true));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void MakeComparisonZCh(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius){
  auto plot = new SpectrumPanel(Form("zchR%02d", int(radius*10.)),Form("z_ch for jets with R=%.1f", radius),radius, THnSparseHandler::kZCh);
  plot->SetFrame(Frame("z_{ch}"," 1/N_{jet} dN/dz_{ch}",{{0., 1., 1e-6, 1.}}, true));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void MakeRatioENe(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius) {
  auto plot = new RatioPanel(Form("RatioSIMEneR%02d", int(radius*10.)),Form("Ratio E_ne for jets with R=%.1f", radius),radius, THnSparseHandler::kENe);
  plot->SetFrame(Frame("E_{ne} (GeV/c)","Ratio to INT7_MC",{{0., 200., 0.5, 1.5}}, false));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void MakeRatioZNe(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius) {
  auto plot = new RatioPanel(Form("RatioSIMzneR%02d", int(radius*10.)),Form("Ratio z_ne for jets with R=%.1f", radius),radius, THnSparseHandler::kZNe);
  plot->SetFrame(Frame("E_{ne} (GeV)","Ratio to INT7_MchC",{{0., 1., 0.5, 1.5}}, false));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void MakeComparisonENe(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius){
  auto plot = new SpectrumPanel(Form("EneR%02d", int(radius*10.)),Form("E_ne for jets with R=%.1f", radius),radius, THnSparseHandler::kENe);
  plot->SetFrame(Frame("E_{ne} (GeV)"," 1/N_{jet} dN/dE_{ne} ((GeV)^{-1})",{{0., 200., 1e-6, 1.}}, true));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void MakeComparisonZNe(std::map<std::string, THnSparseHandler> datamb, std::map<std::string, THnSparseHandler> datatrg, std::map<std::string, THnSparseHandler> mc, double radius){
  auto plot = new SpectrumPanel(Form("zneR%02d", int(radius*10.)),Form("z_ne for jets with R=%.1f", radius),radius, THnSparseHandler::kZNe);
  plot->SetFrame(Frame("z_{ne}"," 1/N_{jet} dN/dz_{ne}",{{0., 1., 1e-6, 1.}}, true));
  plot->SetDataMB(datamb);
  plot->SetDataTrg(datatrg);
  plot->SetMCMB(mc);
  plot->CreatePanel();
}

void makeConstituentComparison(const char *datamb, const char *datatrg, const char *mcfile){
  std::array<double, 2> jetradii = {{0.2, 0.4}};
  for(auto r : jetradii) {
    auto mbch = ExtractDataFromFile(datamb, r, true, true);
    auto trgch = ExtractDataFromFile(datatrg, r, false, true);
    auto mcch = ExtractDataFromFile(mcfile, r, true, true);
    MakeComparisonPtCh(mbch, trgch, mcch, r);
    MakeRatioPtCh(mbch, trgch, mcch, r);
    MakeComparisonZCh(mbch, trgch, mcch, r);
    MakeRatioZCh(mbch, trgch, mcch, r);
    auto mbne = ExtractDataFromFile(datamb, r, true, false);
    auto trgne = ExtractDataFromFile(datatrg, r, false, false);
    auto mcne = ExtractDataFromFile(mcfile, r, true, false);
    MakeComparisonENe(mbne, trgne, mcne, r);
    MakeRatioENe(mbne, trgne, mcne, r);
    MakeComparisonZNe(mbne, trgne, mcne, r);
    MakeRatioZNe(mbne, trgne, mcne, r);
  }
}