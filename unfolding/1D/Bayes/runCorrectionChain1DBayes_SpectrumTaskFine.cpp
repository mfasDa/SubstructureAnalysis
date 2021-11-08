#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/roounfold.C"
#include "../../../helpers/math.C"
#include "../../../helpers/unfolding.C"
#include "../../binnings/binningPt1D.C"

struct unfoldingResults {
    int fReg;
    TH1 *fUnfolded;
    TH1 *fNormalized;
    TH1 *fBackfolded;
    TH1 *fUnfoldedClosure;
    TH2 *fPearson;
    TH2 *fPearsonClosure;

    bool operator<(const unfoldingResults &other) const { return fReg < other.fReg; }
    bool operator==(const unfoldingResults &other) const { return fReg == other.fReg; }
};

std::pair<double, TH1 *> getSpectrumAndNorm(TFile &reader, double R, const std::string_view trigger, const std::string_view triggercluster) {
    int clusterbin = 0;
    if(triggercluster == "ANY") clusterbin = 1;
    else if(triggercluster == "CENT") clusterbin = 2;
    else if(triggercluster == "CENTNOTRD") clusterbin = 3;
    reader.cd(Form("JetSpectrum_FullJets_R%02d_%s", int(R * 10.), trigger.data()));
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hsparse = static_cast<THnSparse *>(histos->FindObject("hJetTHnSparse"));
    hsparse->GetAxis(5)->SetRange(clusterbin, clusterbin);
    auto rawspectrum = hsparse->Projection(1);
    rawspectrum->SetName(Form("RawJetSpectrum_FullJets_R%02d_%s_%s", int(R*10.), trigger.data(), triggercluster.data()));
    rawspectrum->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    auto norm = hnorm->GetBinContent(clusterbin);
    // calculate bin0 correction
    auto evselhistos = static_cast<TList *>(histos->FindObject("StandardRun2ppEventCuts"));
    auto evhist = static_cast<TH1 *>(evselhistos->FindObject("fNormalisationHist"));
    auto bin0correction = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality"));
    norm *= bin0correction;

    return {norm, rawspectrum};
}

TH2 *getResponseMatrix(TFile &reader, double R, int splitstatus = 0) {
    reader.cd(Form("EnergyScaleResults_FullJet_R%02d_INT7", int(R*10.)));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hsparse = std::unique_ptr<THnSparse>(static_cast<THnSparse *>(histlist->FindObject("hPtCorr")));
    if(splitstatus > 0) hsparse->GetAxis(4)->SetRange(splitstatus, splitstatus);
    auto rawresponse = hsparse->Projection(0,1);
    if(splitstatus > 0) hsparse->GetAxis(4)->SetRange(1, 2);
    rawresponse->SetDirectory(nullptr);
    rawresponse->SetNameTitle(Form("Rawresponse_R%02d", int(R*10.)), Form("Raw response R=%.1f", R));
    return rawresponse;
}

double getCENTNOTRDCorrection(TFile &reader){
    reader.cd("JetSpectrum_FullJets_R02_EJ1");
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    return hnorm->GetBinContent(3)/hnorm->GetBinContent(2);
}

TH1 *makeTriggerEfficiency(TFile &mcreader, double R, const std::string_view trigger){
    std::unique_ptr<TH1> mbref(getSpectrumAndNorm(mcreader, R, "INT7", "ANY").second),
                         trgspec(getSpectrumAndNorm(mcreader, R, trigger, "ANY").second);
    auto eff = histcopy(trgspec.get());
    eff->SetNameTitle(Form("TriggerEfficiency_%s_R%02d", trigger.data(), int(R * 10.)), Form("Trigger efficiency for %s for R=%.1f", trigger.data(), R));
    eff->SetDirectory(nullptr);
    eff->Divide(trgspec.get(), mbref.get(), 1., 1., "b");
    return eff;
}

TH1 *makeCombinedRawSpectrum(const TH1 &mb, const TH1 &triggered, double ptswap){
    auto combined = histcopy(&mb);
    combined->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(combined->GetXaxis()->FindBin(ptswap), combined->GetXaxis()->GetNbins()+1)) {
        combined->SetBinContent(b, triggered.GetBinContent(b));
        combined->SetBinError(b, triggered.GetBinError(b));
    }
    return combined;
}

void runCorrectionChain1DBayes_SpectrumTaskFine(const std::string_view datafile, const std::string_view mcfile){
    std::unique_ptr<TFile> datareader(TFile::Open(datafile.data(), "READ")),
                           mcreader(TFile::Open(mcfile.data(), "READ")),
                           writer(TFile::Open("correctedBayes_fine.root", "RECREATE"));
    auto binningpart = getJetPtBinningNonLinTrueFineLow(),
         binningdet = getJetPtBinningNonLinSmearFineLow();
    auto centnotrdcorrection = getCENTNOTRDCorrection(*datareader);
    RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovToy;
    const double kSizeEmcalPhi = 1.8873487,
                 kSizeEmcalEta = 1.4;
    double crosssection = 57.8;
    for(double radius = 0.2; radius <= 0.6; radius += 0.1) {
        std::cout << "Doing jet radius " << radius << std::endl;
        auto mbspectrum = getSpectrumAndNorm(*datareader, radius, "INT7", "ANY"),
             ej1spectrum = getSpectrumAndNorm(*datareader, radius, "EJ1", "CENTNOTRD"),
             ej2spectrum = getSpectrumAndNorm(*datareader, radius, "EJ2", "CENT");
        auto trgeffej1 = makeTriggerEfficiency(*mcreader, radius, "EJ1"),
             trgeffej2 = makeTriggerEfficiency(*mcreader, radius, "EJ2");

        // apply CENTNOTRD correction
        ej1spectrum.second->Scale(1./centnotrdcorrection);

        // Correct for the trigger efficiency
        ej1spectrum.second->Divide(trgeffej1);
        ej2spectrum.second->Divide(trgeffej2);

        // Rebin all raw level histograms
        std::unique_ptr<TH1> mbrebinned(mbspectrum.second->Rebin(binningdet.size()-1, "mbrebinned", binningdet.data())),
                             ej1rebinned(ej1spectrum.second->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data()));
        auto hraw = makeCombinedRawSpectrum(*mbrebinned, *ej1rebinned, 80.);
        hraw->SetNameTitle(Form("hraw_R%02d", int(radius * 10.)), Form("Raw Level spectrum R=%.1f", radius));
        hraw->Scale(crosssection/mbspectrum.first);

        // Get the response matrix
        auto rawresponse = getResponseMatrix(*mcreader, radius);
        rawresponse->SetName(Form("%s_fine", rawresponse->GetName()));
        rawresponse->Scale(40e6);   // undo scaling with the number of trials
        auto rebinnedresponse  = makeRebinned2D(rawresponse, binningdet, binningpart);
        rebinnedresponse->SetName(Form("%s_standard", rebinnedresponse->GetName()));
        std::unique_ptr<TH1> truefulltmp(rawresponse->ProjectionY()),
                             truetmp(rebinnedresponse->ProjectionY());
        auto truefull = truefulltmp->Rebin(binningpart.size() - 1, Form("truefull_R%02d", int(radius*10.)), binningpart.data());
        truefull->SetDirectory(nullptr);
        auto effkine = truetmp->Rebin(binningpart.size()-1, Form("effKine_R%02d", int(radius*10.)), binningpart.data());
        effkine->SetDirectory(nullptr);
        effkine->Divide(effkine, truefull, 1., 1., "b");

        // split the response matrix for the closure test
        auto responseclosure = getResponseMatrix(*mcreader, radius, 1);
        responseclosure->SetName(Form("%s_fine", responseclosure->GetName()));
        std::unique_ptr<TH2> truthclosure(getResponseMatrix(*mcreader, radius, 2));
        truthclosure->SetName("truthclosure");
        responseclosure->Scale(40e6);
        TH1 *priorsclosuretmp(responseclosure->ProjectionY()),
            *detclosuretmp(truthclosure->ProjectionX()),
            *partclosuretmp(truthclosure->ProjectionY());
        auto priorsclosure = priorsclosuretmp->Rebin(binningpart.size()-1, Form("priorsclosure_R%02d", int(radius*10.)), binningpart.data()),
             detclosure = detclosuretmp->Rebin(binningdet.size()-1, Form("detclosure_R%02d", int(radius*10.)), binningdet.data()),
             partclosure = partclosuretmp->Rebin(binningpart.size()-1, Form("partclosure_R%02d", int(radius*10.)), binningpart.data());
        priorsclosure->SetDirectory(nullptr);
        detclosure->SetDirectory(nullptr);
        partclosure->SetDirectory(nullptr);
        auto rebinnedresponseclosure = makeRebinned2D(responseclosure, binningdet, binningpart);
        rebinnedresponseclosure->SetName(Form("%s_closure", rebinnedresponseclosure->GetName()));

        RooUnfoldResponse responsematrix(nullptr, truefull, rebinnedresponse),
                          responsematrixClosure(nullptr, priorsclosure, rebinnedresponseclosure);
        responsematrix.UseOverflow(false);
        responsematrixClosure.UseOverflow(false);
        std::set<unfoldingResults> unfolding_results;
        auto acceptance = (kSizeEmcalPhi - 2 * radius) * (kSizeEmcalEta - 2 * radius) / (TMath::TwoPi());
        for(auto ireg : ROOT::TSeqI(1, 30)) {
            std::cout << "[Bayes unfolding] Regularization " << ireg << "\n================================================================\n";
            std::cout << "[Bayes unfolding] Running unfolding" << std::endl;
            RooUnfoldBayes unfolder(&responsematrix, hraw, ireg);
            auto specunfolded = unfolder.Hreco(errorTreatment);
            specunfolded->SetNameTitle(Form("unfolded_reg%d", ireg), Form("Unfolded jet spectrum R=%.1f reg %d", radius, ireg));
            specunfolded->SetDirectory(nullptr);
            auto backfolded = MakeRefolded1D(hraw, specunfolded, responsematrix);
            backfolded->SetNameTitle(Form("backfolded_reg%d", ireg), Form("back-folded jet spectrum R=%.1f reg %d", radius, ireg));
            backfolded->SetDirectory(nullptr);
            normalizeBinWidth(specunfolded);
            auto specnormalized = static_cast<TH1 *>(specunfolded->Clone(Form("normalizedReg%d", ireg)));
            specnormalized->SetNameTitle(Form("normalized_reg%d", ireg), Form("Normalized jet spectrum R=%.1f reg %d", radius, ireg));
            specnormalized->SetDirectory(nullptr);
            specnormalized->Scale(1. / (acceptance));

            // run closure test
            std::cout << "[Bayes unfolding] Running closure test" << std::endl;
            RooUnfoldBayes unfolderClosure(&responsematrixClosure, detclosure, ireg);
            auto specunfoldedClosure = unfolderClosure.Hreco(errorTreatment);
            specunfoldedClosure->SetDirectory(nullptr);
            specunfoldedClosure->SetNameTitle(Form("unfoldedClosure_reg%d", ireg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d", radius, ireg));
            unfolding_results.insert({ireg, specunfolded, specnormalized, backfolded, specunfoldedClosure, 
                                        CorrelationHist1D(unfolder.Ereco(), Form("PearsonReg%d", ireg), Form("Pearson coefficients regularization %d", ireg)),
                                        CorrelationHist1D(unfolderClosure.Ereco(), Form("PearsonClosureReg%d", ireg), Form("Pearson coefficients of the closure test regularization %d", ireg))});
        }

        // Write everything to file
        std::cout << "[Bayes unfolding] Writing to file ..." << std::endl;
        writer->mkdir(Form("R%02d", int(radius*10)));
        writer->cd(Form("R%02d", int(radius*10)));
        auto basedir = static_cast<TDirectory *>(gDirectory);
        basedir->mkdir("rawlevel");
        basedir->cd("rawlevel");
        mbspectrum.second->Write();
        ej1spectrum.second->Write();
        ej2spectrum.second->Write();
        trgeffej1->Write();
        trgeffej2->Write();
        hraw->Write();
        auto hnorm = new TH1F("hNorm", "Norm", 1, 0.5, 1.5);
        hnorm->SetBinContent(1, mbspectrum.first);
        hnorm->Write();
        auto hcntcorr = new TH1F("hCENTNOTRDcorrection", "CENTNOTRD correction", 1, 0.5, 1.5);
        hcntcorr->SetBinContent(1, centnotrdcorrection);
        hcntcorr->Write();
        basedir->mkdir("response");
        basedir->cd("response");
        rawresponse->Write();
        rebinnedresponse->Write();
        truefull->Write();
        effkine->Write();
        basedir->mkdir("closuretest");
        basedir->cd("closuretest");
        priorsclosure->Write("priorsclosure");
        detclosure->Write("detclosure");
        partclosure->Write("partclosure");
        responseclosure->Write("responseClosureFine");
        rebinnedresponseclosure->Write("responseClosureRebinned");
        for(auto reg : unfolding_results){
            basedir->mkdir(Form("reg%d", reg.fReg));
            basedir->cd(Form("reg%d", reg.fReg));
            if(reg.fUnfolded) reg.fUnfolded->Write();
            if(reg.fNormalized) reg.fNormalized->Write();
            if(reg.fBackfolded) reg.fBackfolded->Write();
            if(reg.fPearson) reg.fPearson->Write();
            if(reg.fUnfoldedClosure) reg.fUnfoldedClosure->Write();
            if(reg.fPearsonClosure) reg.fPearsonClosure->Write();
        }
        std::cout << "[Bayes unfolding] Radius " << radius << " done ..." << std::endl;
    }
}