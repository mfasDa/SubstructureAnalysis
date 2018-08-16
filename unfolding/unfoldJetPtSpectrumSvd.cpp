#ifndef __CLING__
#include <vector>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <ROOT/RDataFrame.hxx>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TTreeReader.h>
#include <TRandom.h>

#include "RooUnfoldResponse.h"
#include "RooUnfoldSvd.h"
#include "TSVDUnfold_local.h"
#endif

#include "../helpers/filesystem.C"
#include "../helpers/math.C"
#include "../helpers/root.C"
#include "../helpers/string.C"
#include "../helpers/unfolding.C"
#include "../helpers/substructuretree.C"

#include "binnings/binningPt1D.C"

void unfoldJetPtSpectrumSvd(const std::string_view filedata, const std::string_view filemc){
  ROOT::EnableImplicitMT(8);
  double ptmin = 20., ptmax = 120.;
  auto binningdet = getJetPtBinningNonLinSmear(), binningpart = getJetPtBinningNonLinTrue();
  std::string outfilename = Form("unfoldedEnergySvd_%s.root", getFileTag(filedata).data());

  // read data
  ROOT::RDataFrame df(GetNameJetSubstructureTree(filedata), filedata);
  auto model = df.Filter(Form("NEFRec < 0.98 &&  PtJetRec > %.1f && PtJetRec < %.1f", ptmin, ptmax)).Histo1D({"hraw", "raw spectrum", static_cast<int>(binningdet.size() -1), binningdet.data()}, "PtJetRec");
  auto hraw = model.GetPtr();

  // read MC
  TH1 *htrue = new TH1D("htrue", "true spectrum", binningpart.size()-1, binningpart.data()),
      *hsmeared = new TH1D("hsmeared", "det mc", binningdet.size()-1, binningdet.data()), 
      *hsmearedClosure = new TH1D("hsmearedClosure", "det mc (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueClosure = new TH1D("htrueClosure", "true spectrum (for closure test)", binningdet.size() - 1, binningdet.data()),
      *htrueFull = new TH1D("htrueFull", "non-truncated true spectrum", binningpart.size() - 1, binningpart.data()),
      *htrueFullClosure = new TH1D("htrueFullClosure", "non-truncated true spectrum (for closure test)", binningpart.size() - 1, binningpart.data()),
      *hpriorsClosure = new TH1D("hpriorsClosure", "non-truncated true spectrum (for closure test, same jets as repsonse matrix)", binningpart.size() - 1, binningpart.data());
  TH2 *responseMatrix = new TH2D("responseMatrix", "response matrix", binningdet.size()-1, binningdet.data(), binningpart.size()-1, binningpart.data()),
      *responseMatrixClosure = new TH2D("responseMatrixClosure", "response matrix (for closure test)", binningdet.size()-1, binningdet.data(), binningpart.size()-1, binningpart.data());

  {
    TRandom closuresplit;
    std::unique_ptr<TFile> fread(TFile::Open(filemc.data(), "READ"));
    TTreeReader mcreader(GetDataTree(*fread));
    TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                              ptsim(mcreader, "PtJetSim"), 
                              nefrec(mcreader, "NEFRec"),
                              weight(mcreader, "PythiaWeight");
    bool closureUseSpec;
    for(auto en : mcreader){
      if(*nefrec > 0.98) continue;
      double rdm = closuresplit.Uniform();
      closureUseSpec = (rdm < 0.2);
      htrueFull->Fill(*ptsim, *weight);
      if(closureUseSpec) htrueFullClosure->Fill(*ptsim, *weight);
      else hpriorsClosure->Fill(*ptsim, *weight);
      if(*ptrec > ptmin && *ptrec < ptmax){
        htrue->Fill(*ptsim, *weight);
        hsmeared->Fill(*ptrec, *weight);
        responseMatrix->Fill(*ptrec, *ptsim, *weight);
        if(closureUseSpec) {
          hsmearedClosure->Fill(*ptrec, *weight);
          htrueClosure->Fill(*ptsim, *weight);
        } else {
          responseMatrixClosure->Fill(*ptrec, *ptsim, *weight);
        }
      }
    }
  }

  // Calculate kinematic efficiency
  auto effKine = histcopy(htrue);
  effKine->SetDirectory(nullptr);
  effKine->SetName("effKine");
  effKine->Divide(effKine, htrueFull, 1., 1., "b");

  // Normalize response matrices
  //Normalize2D(responseMatrix); Normalize2D(responseMatrixClosure);  

  // Build response
  RooUnfoldResponse response(nullptr, htrueFull, responseMatrix, "response"), responseClosure(nullptr, hpriorsClosure, responseMatrixClosure, "responseClosure");

  std::unique_ptr<TFile> writer(TFile::Open(outfilename.data(), "RECREATE"));
  htrueFull->Write();
  htrueFullClosure->Write();
  hpriorsClosure->Write();
  htrue->Write();
  htrueClosure->Write();
  hsmeared->Write();
  hsmearedClosure->Write();
  responseMatrix->Write();
  responseMatrixClosure->Write();
  hraw->Write();
  effKine->Write();

  RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovToy;//ariance;
  for(auto reg : ROOT::TSeqI(1, hraw->GetXaxis()->GetNbins())){
    std::cout << "Regularization " << reg << "\n================================================================\n";
    std::cout << "Running unfolding" << std::endl;
    RooUnfoldSvd unfolder(&response, hraw, reg);
    auto unfolded = unfolder.Hreco(errorTreatment);
    unfolded->SetName(Form("unfoldedReg%d", reg));
    TH1 *dvec(nullptr);
    if(auto imp = unfolder.Impl()){
      dvec = imp->GetD();
      dvec->SetName(Form("dvectorReg%d", reg));
    }
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "Running MC closure test" << std::endl;
    RooUnfoldSvd unfolderClosure(&responseClosure, hsmearedClosure, reg, 1000, "unfolderClosure", "unfolderClosure");
    auto unfoldedClosure = unfolderClosure.Hreco(errorTreatment);
    unfoldedClosure->SetName(Form("unfoldedClosureReg%d", reg));
    TH1 * dvecClosure(nullptr);
    if(auto imp = unfolderClosure.Impl()){
      dvecClosure = imp->GetD();
      dvecClosure->SetName(Form("dvectorClosureReg%d", reg));
    }

    // back-folding test
    auto backfolded = MakeRefolded1D(hraw, unfolded, response);
    backfolded->SetName(Form("backfolded_reg%d", reg));
    auto backfoldedClosure = MakeRefolded1D(hsmearedClosure, unfoldedClosure, responseClosure);
    backfoldedClosure->SetName(Form("backfoldedClosure_reg%d", reg));

    writer->mkdir(Form("regularization%d", reg));
    writer->cd(Form("regularization%d", reg));
    unfolded->Write();
    if(dvec) dvec->Write();
    unfoldedClosure->Write();
    if(dvecClosure) dvecClosure->Write();
    backfolded->Write();
    backfoldedClosure->Write();
  }
}