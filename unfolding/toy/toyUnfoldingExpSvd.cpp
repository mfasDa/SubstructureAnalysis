#ifndef __CLING__
#include <memory>
#include <mutex>
#include <thread>
#include <sstream>
#include <string>
#include <ROOT/TSeq.hxx>
#include <TFile.h>
#include <TF1.h>
#include <TH1.h>
#include <TH2.h>
#include <TRandom.h>
#include <TROOT.H>

#include "RooUnfoldResponse.h"
#include "RooUnfoldSvd.h"
#include "TSVDUnfold_local.h"
#endif

class smearer{
public:
  smearer() = default;
  ~smearer() noexcept = default;

  Double_t smaer(Double_t ptin){
    double sigma = 0.2 * ptin;
    return std::max(fGen.Gaus(ptin, sigma), 0.);
  }

  void SetSeed(ULong_t seed) { fGen.SetSeed(seed); }

private:
  TRandom fGen;
};

std::vector<double> makeLinearBinning(double ptmin, double ptmax, double binwidth) {
  std::vector<double> binning;
  for(auto b = ptmin; b <= ptmax; b += binwidth) binning.emplace_back(b);
  return binning;
}

void toyUnfoldingExpSvd(){
  ROOT::EnableThreadSafety();

  Double_t ptmin = 20., ptmax = 120., ptpartmin = 0., ptpartmax = 400;
  auto binningsmear = makeLinearBinning(ptmin, ptmax, 2.),
       binningtrue = makeLinearBinning(ptpartmin, ptpartmax, 10.);

  TH1 *hRaw = new TH1D("hRaw", "simulated raw pt spectrum", binningsmear.size()-1, binningsmear.data()),
      *hTrue = new TH1D("hTrue", "simulated true pt spectrum (truncated at assoc smeared level)", binningtrue.size()-1, binningtrue.data()),
      *hSmeared = new TH1D("hSmeared", "simulated smeared true pt spectrum", binningsmear.size()-1, binningsmear.data()),
      *hTrueFull = new TH1D("hTrueFull", "simulated true pt spectrum (non-truncated)", binningtrue.size()-1, binningtrue.data());
  TH2 *hResponse = new TH2D("hResponse", "Response matrix", binningsmear.size()-1, binningsmear.data(), binningtrue.size()-1, binningtrue.data());


  // Fill measured
  std::cout << "Filling data and response ... \n";
  TF1 model("model", "1e-6 * TMath::Power(50/x, 5)", 0., 1000.); 
  std::mutex ptgenmutex;
  std::vector<std::thread> rawthreads;
  std::mutex rawmutex;
  auto datafiller = [&](int threadid){
    std::cout << "Starting data thread " << threadid << " ...\n";
    smearer ptsmearer;
    ptsmearer.SetSeed(threadid);
    for(auto istep : ROOT::TSeqI(0, 100000000)){
      double truept;
      {
        std::unique_lock<std::mutex> ptgenlock(ptgenmutex);
        truept = model.GetRandom(0., 200.);
      }
      auto smearedpt = ptsmearer.smaer(truept);
      if(smearedpt < ptmin || smearedpt  > ptmax) continue;
      {
        std::unique_lock<std::mutex> filllock(rawmutex);
        hRaw->Fill(smearedpt);
      }
    }
    std::cout << "Data thread " << threadid << " done ...\n";
  };
  for(auto ithread : ROOT::TSeqI(0, 20)) rawthreads.emplace_back(datafiller, ithread);

  // Fill true and repsonse
  std::vector<std::thread> mcthreads;
  std::mutex mcmutex;
  auto mcfiller = [&](int threadid){
    std::cout << "Starting MC thread " << threadid << "...\n";
    smearer ptsmearer;
    ptsmearer.SetSeed(threadid + 100);
    for(auto istep : ROOT::TSeqI(0, 100000000)){
      double truept;
      {
        std::unique_lock<std::mutex> ptgenlock(ptgenmutex);
        truept = model.GetRandom(ptpartmin, ptpartmax);
      }
      auto smearedpt = ptsmearer.smaer(truept);
      hTrueFull->Fill(truept);
      if(smearedpt < ptmin || smearedpt  > ptmax) continue;
      {
        std::unique_lock<std::mutex> filllock(rawmutex);
        hTrue->Fill(truept);
        hSmeared->Fill(smearedpt);
        hResponse->Fill(smearedpt, truept);
      }
    }
    std::cout << "MC thread " << threadid << " done ...\n";
  };
  for(auto jthread : ROOT::TSeqI(0, 20)) mcthreads.emplace_back(mcfiller, jthread);

  // join filling threads
  for(auto &rt : rawthreads) rt.join();
  for(auto &mt : mcthreads) mt.join();
  std::cout << "Filling data and response done ...\n";

  // calculate kinematic efficiency
  std::cout << "Calculating kinematic efficiency ... \n";
  auto heff = new TH1D(*static_cast<TH1D *>(hTrue));
  heff->SetNameTitle("hEffKine", "Kinematic efficiency");
  heff->Divide(heff, hTrueFull, 1., 1., "b");

  std::unique_ptr<TFile> writer(TFile::Open("toysvd.root", "RECREATE"));
  hRaw->Write();
  hTrue->Write();
  hTrueFull->Write();
  hSmeared->Write();
  heff->Write();
  hResponse->Write();

  std::cout << "Start unfolding ..." << std::endl;
  std::cout << "=================================" << std::endl;
  RooUnfoldResponse response(nullptr, hTrueFull, hResponse);
  RooUnfold::ErrorTreatment err = RooUnfold::kCovToy;
  for(auto ireg : ROOT::TSeqI(1, hRaw->GetXaxis()->GetNbins())){
    std::cout << "Doing regularization " << ireg << std::endl;
    RooUnfoldSvd unfolder(&response, hRaw, ireg);
    auto unfolded = unfolder.Hreco(err);
    unfolded->SetName(Form("unfolded_reg%d", ireg));
    auto dvec = unfolder.Impl()->GetD();
    dvec->SetName(Form("devtor_reg%d", ireg));

    std::stringstream dirname;
    dirname << "regularization" << ireg;
    writer->mkdir(dirname.str().data());
    writer->cd(dirname.str().data());
    unfolded->Write();
    dvec->Write();
  }
}