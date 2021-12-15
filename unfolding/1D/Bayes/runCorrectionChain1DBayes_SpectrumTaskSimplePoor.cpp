#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/roounfold.C"
#include "../../../helpers/math.C"
#include "../../../helpers/unfolding.C"
#include "../../binnings/binningPt1D.C"

class Trials {
public:
    Trials(TH1 *hist) { fHistogram = hist; };
    ~Trials() { delete fHistogram; }

    double getMaxTrials() const {
        auto bins = getBins();
        return *std::max_element(bins.begin(), bins.end());
    }

    double getAverageTrials() const {
        auto bins = getBins();
        return TMath::Mean(bins.begin(), bins.end());
    }

    double getTrialsFit() const {
        TF1 model("meanntrials", "pol0", 0., 100.);
        fHistogram->Fit(&model, "N", "", fHistogram->GetXaxis()->GetBinLowEdge(2), fHistogram->GetXaxis()->GetBinUpEdge(fHistogram->GetXaxis()->GetNbins()+1));
        return model.GetParameter(0);
    }

private:
    std::vector<double> getBins() const {
        std::vector<double> result;
        for(int ib = 0; ib < fHistogram->GetXaxis()->GetNbins(); ib++) {
            auto entries  = fHistogram->GetBinContent(ib+1);
            if(TMath::Abs(entries) > DBL_EPSILON) {
                // filter 0
                result.emplace_back(entries);
            }
        }
        return result;
    }

    TH1 *fHistogram;
};

struct SpectrumAndNorm {
    TH1 *fSpectrum;
    double fEventCount;
    double fEventCountAbsolute;
    double fVertexFindingEfficiency;

    double getNorm() const { return fEventCount / fVertexFindingEfficiency; }
};

struct unfoldingResults {
    int fReg;
    TH1 *fUnfolded;
    TH1 *fNormalized;
    TH1 *fNormalizedNoEff;
    TH1 *fBackfolded;
    TH1 *fUnfoldedClosureNoEff;
    TH1 *fUnfoldedClosure;
    TH1 *fDvector;
    TH1 *fDvectorClosure;
    TH2 *fPearson;
    TH2 *fPearsonClosure;

    bool operator<(const unfoldingResults &other) const { return fReg < other.fReg; }
    bool operator==(const unfoldingResults &other) const { return fReg == other.fReg; }
};

struct UnfoldingConfiguration {
    int fReg;
    double fRadius;
    TH1 *fRaw;
    TH1 *fJetFindingEff;
    RooUnfoldResponse *fResponseMatrix;
    TH1 *fDetLevelClosure;
    RooUnfoldResponse *fResponseMatrixClosure;
    TH1 *fJetFindingEffClosure;
};

class UnfoldingPool {
public:
    UnfoldingPool() = default;
    ~UnfoldingPool() = default;

    void InsertWork(UnfoldingConfiguration config) {
        std::lock_guard<std::mutex> insert_lock(fAccessLock);
        fData.push(config);
    }

    UnfoldingConfiguration next() {
        std::lock_guard<std::mutex> pop_lock(fAccessLock);
        UnfoldingConfiguration result = fData.front();
        fData.pop();
        return result;
    }

    bool empty() { return fData.empty(); }

private:
    std::mutex                              fAccessLock;
    std::queue<UnfoldingConfiguration>      fData;
};

class UnfoldingRunner {
    public:
        UnfoldingRunner(UnfoldingPool *work) : fOutputData(), fInputData(work) {}
        ~UnfoldingRunner() = default;

        void DoWork() {
            while(!fInputData->empty()) {
                fOutputData.push_back(runUnfolding(fInputData->next()));
            }
        }

        const std::vector<unfoldingResults> &getUnfolded() const { return fOutputData; }

    private:
        unfoldingResults runUnfolding(const UnfoldingConfiguration &config){
            RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovToy;
            const double kSizeEmcalPhi = 1.8873487,
                         kSizeEmcalEta = 1.4;
            auto acceptance = (kSizeEmcalPhi - 2 * config.fRadius) * (kSizeEmcalEta - 2 * config.fRadius) / (TMath::TwoPi());
            std::cout << "[Bayes unfolding] Regularization " << config.fReg << "\n================================================================\n";
            std::cout << "[Bayes unfolding] Running unfolding" << std::endl;
            RooUnfoldBayes unfolder(config.fResponseMatrix, config.fRaw, config.fReg);
            auto specunfolded = unfolder.Hreco(errorTreatment);
            specunfolded->SetNameTitle(Form("unfolded_reg%d", config.fReg), Form("Unfolded jet spectrum R=%.1f reg %d", config.fRadius, config.fReg));
            specunfolded->SetDirectory(nullptr);
            auto backfolded = MakeRefolded1D(config.fRaw, specunfolded, *config.fResponseMatrix);
            backfolded->SetNameTitle(Form("backfolded_reg%d", config.fReg), Form("back-folded jet spectrum R=%.1f reg %d", config.fRadius, config.fReg));
            backfolded->SetDirectory(nullptr);
            specunfolded->Scale(1., "width");
            auto specnormalizedNoEff = static_cast<TH1 *>(specunfolded->Clone(Form("normalizedNoEffReg%d", config.fReg)));
            specnormalizedNoEff->SetNameTitle(Form("normalizedNoEff_reg%d", config.fReg), Form("Normalized jet spectrum R=%.1f reg %d, no correction for jet finding efficiency", config.fRadius, config.fReg));
            specnormalizedNoEff->SetDirectory(nullptr);
            specnormalizedNoEff->Scale(1. / (acceptance));
            auto specnormalized = static_cast<TH1 *>(specnormalizedNoEff->Clone(Form("normalizedReg%d", config.fReg)));
            specnormalized->SetNameTitle(Form("normalized_reg%d", config.fReg), Form("Normalized jet spectrum R=%.1f reg %d", config.fRadius, config.fReg));
            specnormalized->SetDirectory(nullptr);
            specnormalized->Divide(config.fJetFindingEff);

            // run closure test
            std::cout << "[Bayes unfolding] Running closure test" << std::endl;
            RooUnfoldBayes unfolderClosure(config.fResponseMatrixClosure, config.fDetLevelClosure, config.fReg);
            auto specunfoldedClosureNoEff = unfolderClosure.Hreco(errorTreatment);
            specunfoldedClosureNoEff->SetDirectory(nullptr);
            specunfoldedClosureNoEff->SetNameTitle(Form("unfoldedClosureNoEff_reg%d", config.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d, no correction for jet finding efficiency", config.fRadius, config.fReg));
            specunfoldedClosureNoEff->Scale(1., "width");
            auto specunfoldedClosure = static_cast<TH1 *>(specunfoldedClosureNoEff->Clone());
            specunfoldedClosure->SetDirectory(nullptr);
            specunfoldedClosure->SetNameTitle(Form("unfoldedClosure_reg%d", config.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d", config.fRadius, config.fReg));
            if(config.fJetFindingEffClosure)
                specunfoldedClosure->Divide(config.fJetFindingEffClosure);
            else
                specunfoldedClosure->Divide(config.fJetFindingEff);

            return {config.fReg, specunfolded, specnormalizedNoEff, specnormalized, backfolded, specunfoldedClosureNoEff, specunfoldedClosure, nullptr, nullptr,
                    CorrelationHist1D(unfolder.Ereco(), Form("PearsonReg%d", config.fReg), Form("Pearson coefficients regularization %d", config.fReg)),
                    CorrelationHist1D(unfolderClosure.Ereco(), Form("PearsonClosureReg%d", config.fReg), Form("Pearson coefficients of the closure test regularization %d", config.fReg))};
        }

        std::vector<unfoldingResults>   fOutputData;
        UnfoldingPool                   *fInputData;
};

TH1 *makeRebinnedSafe(const TH1 *input, const char *newname, const std::vector<double> binning) {
    std::unique_ptr<TH1> rebinhelper(histcopy(input));
    auto rebinned = rebinhelper->Rebin(binning.size()-1, newname, binning.data());
    return rebinned;
}

SpectrumAndNorm getSpectrumAndNorm(TFile &reader, int R, const std::string_view trigger, const std::string_view triggercluster, const std::string_view sysvar) {
    int clusterbin = 0;
    if(triggercluster == "ANY") clusterbin = 1;
    else if(triggercluster == "CENT") clusterbin = 2;
    else if(triggercluster == "CENTNOTRD") clusterbin = 3;
    std::stringstream dirnamebuilder, histnamebuilder;
    dirnamebuilder << "JetSpectrum_FullJets_R" << std::setw(2) << std::setfill('0') << R << "_" << trigger;
    histnamebuilder << "RawJetSpectrum_FullJets_R" <<  std::setw(2) << std::setfill('0') << R << "_" << trigger << "_" << triggercluster;
    if(sysvar.length()) {
        dirnamebuilder << "_" << sysvar;
        histnamebuilder << "_" << sysvar;
    }
    reader.cd(dirnamebuilder.str().data());
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hspecandclass = static_cast<TH2 *>(histos->FindObject("hJetSpectrum"));
    auto rawspectrum = hspecandclass->ProjectionY(histnamebuilder.str().data(), clusterbin, clusterbin, "e");
    rawspectrum->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    auto norm = hnorm->GetBinContent(clusterbin);
    // Get absolute eventCounts
    auto heventcounts = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));
    auto eventcounts = heventcounts->GetBinContent(clusterbin);
    // calculate vertex finding efficiency
    auto evhist = static_cast<TH1 *>(histos->FindObject("fNormalisationHist")); // take the one from the AliAnalysisTaskEmcal directly
    auto vertexfindingeff = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection"));
    SpectrumAndNorm result{rawspectrum, norm, eventcounts, vertexfindingeff};
    return result;
}

TH2 *getResponseMatrix(TFile &reader, int R, const std::string_view sysvar, int closurestatus = 0) {
    std::string closuretag;
    switch(closurestatus) {
    case 0: break;
    case 1: closuretag = "Closure"; break;
    case 2: closuretag = "NoClosure"; break;
    };
    std::string responsematrixbase = "hJetResponseFine";
    std::stringstream responsematrixname;
    responsematrixname << responsematrixbase;
    if(closuretag.length()) responsematrixname <<  closuretag;
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto rawresponse = static_cast<TH2 *>(histlist->FindObject(responsematrixname.str().data()));
    rawresponse->SetDirectory(nullptr);
    rawresponse->SetNameTitle(Form("Rawresponse_R%02d", R), Form("Raw response R=%.1f", double(R)/10.));
    return rawresponse;
}

Trials getTrials(TFile &reader, int R, const std::string_view sysvar) {
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto histtrials = static_cast<TH2 *>(histlist->FindObject("fHistTrials")); 
    histtrials->SetDirectory(nullptr);
    return Trials(histtrials);
}

double getCENTNOTRDCorrection(TFile &reader, const std::string_view sysvar){
    std::stringstream dirnamebuilder;
    dirnamebuilder << "JetSpectrum_FullJets_R02_EJ1";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    return hnorm->GetBinContent(3)/hnorm->GetBinContent(2);
}

TH1 *makeEventCounterHistogram(const char *name, const char *title, double countEJ1, double countEJ2, double countINT7) { 
    auto eventCount = new TH1F(name, title, 3, 0., 3);
    eventCount->GetXaxis()->SetBinLabel(1, "INT7");
    eventCount->GetXaxis()->SetBinLabel(2, "EJ2");
    eventCount->GetXaxis()->SetBinLabel(3, "EJ1");
    eventCount->SetBinContent(1, countINT7);
    eventCount->SetBinContent(2, countEJ2);
    eventCount->SetBinContent(3, countEJ1);
    return eventCount;
}

TH1 *makeTriggerEfficiency(TFile &mcreader, int R, const std::string_view trigger, const std::string_view sysvar, const std::vector<double> *binning = nullptr){
    std::unique_ptr<TH1> mbref(getSpectrumAndNorm(mcreader, R, "INT7", "ANY", sysvar).fSpectrum),
                         trgspec(getSpectrumAndNorm(mcreader, R, trigger, "ANY", sysvar).fSpectrum);
    TH1 *eff = nullptr,
        *numerator = trgspec.get(),
        *denominator = mbref.get();
    std::string histname = Form("TriggerEfficiency_%s_R%02d", trigger.data(), R),
                histtitle = Form("Trigger efficiency for %s for R=%.1f", trigger.data(), double(R)/10.);
    if(binning) {
        // Make rebin on clone, to be on the safe side as the rebin function is not marked const
        std::unique_ptr<TH1> triggeredRebinned(makeRebinnedSafe(trgspec.get(),Form("%s_rebinned", trgspec->GetName()), *binning)); 
        std::unique_ptr<TH1> minbiasRebinned(makeRebinnedSafe(mbref.get(), Form("%s_rebinned", mbref->GetName()), *binning));
        eff = histcopy(triggeredRebinned.get());
        eff->Divide(triggeredRebinned.get(), minbiasRebinned.get(), 1., 1., "b");
        histname += "_rebinned";
        histtitle += " (rebinned)";
    } else {
        eff = histcopy(trgspec.get());
        eff->Divide(trgspec.get(), mbref.get(), 1., 1., "b");
        histname += "_orig";
        histtitle += " (original)";
    }
    eff->SetNameTitle(histname.data(), histtitle.data());
    eff->SetDirectory(nullptr);
    return eff;
}

TH1 *makeCombinedRawSpectrum(const TH1 &mb, const TH1 &ej2, double ej2swap, const TH1 &ej1, double ej1swap){
    auto combined = histcopy(&mb);
    combined->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(combined->GetXaxis()->FindBin(ej2swap), combined->GetXaxis()->FindBin(ej1swap))) {
        std::cout << "[" << combined->GetXaxis()->GetBinLowEdge(b) << " - " << combined->GetXaxis()->GetBinUpEdge(b) << "] Using EJ2" << std::endl;
        combined->SetBinContent(b, ej2.GetBinContent(b));
        combined->SetBinError(b, ej2.GetBinError(b));
    }
    for(auto b : ROOT::TSeqI(combined->GetXaxis()->FindBin(ej1swap), combined->GetXaxis()->GetNbins()+1)) {
        std::cout << "[" << combined->GetXaxis()->GetBinLowEdge(b) << " - " << combined->GetXaxis()->GetBinUpEdge(b) << "] Using EJ1" << std::endl;
        combined->SetBinContent(b, ej1.GetBinContent(b));
        combined->SetBinError(b, ej1.GetBinError(b));
    }
    return combined;
}

TH1* getFullyEfficientTruth(TFile &reader, int R, const std::string_view sysvar, const std::vector<double> binningpart, int closurestatus) {
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    std::stringstream inputname;
    inputname << "hJetSpectrumPartAll";
    if(closurestatus == 1)
        inputname << "NoClosure";
    else if(closurestatus == 2)
        inputname << "Closure";
    auto spectrum = static_cast<TH1 *>(histlist->FindObject(inputname.str().data()));
    if(spectrum) {
        std::stringstream outname, outtitle;
        outname << "hTruthFullyEfficient_R" << std::setw(2) << std::setfill('0') << R;
        outtitle << "Fully efficient true spectrum for R=" << std::setprecision(2) << double(R)/10.;
        if(closurestatus == 1) {
            outname << "_Closure";
            outtitle << " (for closure test)";
        } else if(closurestatus == 2) {
            outname << "_ClosureSample";
            outtitle << " (for closure test)";
        }
        auto rebinned = spectrum->Rebin(binningpart.size()-1, outname.str().data(), binningpart.data());
        rebinned->SetDirectory(nullptr);
        rebinned->SetTitle(outtitle.str().data());
        return rebinned;
    }
    return nullptr;
}

void runCorrectionChain1DBayes_SpectrumTaskSimplePoor(const std::string_view datafile, const std::string_view mcfile, const std::string_view sysvar = "", int radiusSel = -1, bool doMT = false){
    ROOT::EnableThreadSafety();
    std::stringstream outputfile;
    outputfile << "correctedBayes_poor";
    if(sysvar.length()) {
        outputfile << "_" << sysvar;
    }
    if(radiusSel > 0) {
        outputfile << "_R" << std::setfill('0') << std::setw(2) << radiusSel;
    }
    outputfile << ".root";
    std::unique_ptr<TFile> datareader(TFile::Open(datafile.data(), "READ")),
                           mcreader(TFile::Open(mcfile.data(), "READ")),
                           writer(TFile::Open(outputfile.str().data(), "RECREATE"));
    auto binningpart = getJetPtBinningNonLinTruePoor(),
         binningdet = getJetPtBinningNonLinSmearPoor();
    auto centnotrdcorrection = getCENTNOTRDCorrection(*datareader, sysvar);
    double crosssection = 57.8;
    for(auto R : ROOT::TSeqI(2, 7)) {
        double radius = double(R) / 10.;
        if(radiusSel > 0 && R != radiusSel) {
            continue;
        }
        std::cout << "Doing jet radius " << radius << std::endl;
        auto mbspectrum = getSpectrumAndNorm(*datareader, R, "INT7", "ANY", sysvar),
             ej1spectrum = getSpectrumAndNorm(*datareader, R, "EJ1", "CENTNOTRD", sysvar),
             ej2spectrum = getSpectrumAndNorm(*datareader, R, "EJ2", "CENT", sysvar);
        auto trgeffej1 = makeTriggerEfficiency(*mcreader, R, "EJ1", sysvar),
             trgeffej2 = makeTriggerEfficiency(*mcreader, R, "EJ2", sysvar),
             rebinnedTriggerEffEJ1 = makeTriggerEfficiency(*mcreader, R, "EJ1", sysvar, &binningdet),
             rebinnedTriggerEffEJ2 = makeTriggerEfficiency(*mcreader, R, "EJ2", sysvar, &binningdet);

        // apply CENTNOTRD correction
        ej1spectrum.fSpectrum->Scale(1./centnotrdcorrection);

        TH1 *ej1rebinnedUncorrected = makeRebinnedSafe(ej1spectrum.fSpectrum, "ej1rebinnedUncorrected", binningdet),
            *ej2rebinnedUncorrected = makeRebinnedSafe(ej2spectrum.fSpectrum, "ej2rebinnedUncorrected", binningdet);

        // Correct for the trigger efficiency
        ej1spectrum.fSpectrum->Divide(trgeffej1);
        ej2spectrum.fSpectrum->Divide(trgeffej2);

        // Rebin all raw level histograms
        TH1 *mbrebinned(mbspectrum.fSpectrum->Rebin(binningdet.size()-1, "mbrebinned", binningdet.data())),
            *ej1rebinned(ej1spectrum.fSpectrum->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data())),
            *ej2rebinned(ej2spectrum.fSpectrum->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data()));
        auto hraw = makeCombinedRawSpectrum(*mbrebinned, *ej2rebinned, 50., *ej1rebinned, 100.);
        hraw->SetNameTitle(Form("hraw_R%02d", R), Form("Raw Level spectrum R=%.1f", radius));
        hraw->Scale(crosssection/mbspectrum.getNorm());

        // Get the response matrix
        auto rawresponse = getResponseMatrix(*mcreader, R, sysvar);
        auto trials = getTrials(*mcreader, int(radius*10.), sysvar);
        auto mcscale = trials.getMaxTrials();
        rawresponse->SetName(Form("%s_fine", rawresponse->GetName()));
        rawresponse->Scale(mcscale);   // undo scaling with the number of trials
        auto rebinnedresponse  = makeRebinned2D(rawresponse, binningdet, binningpart);
        rebinnedresponse->SetName(Form("%s_standard", rebinnedresponse->GetName()));
        std::unique_ptr<TH1> truefulltmp(rawresponse->ProjectionY()),
                             truetmp(rebinnedresponse->ProjectionY());
        auto truefull = truefulltmp->Rebin(binningpart.size() - 1, Form("truefull_R%02d", R), binningpart.data());
        truefull->SetDirectory(nullptr);
        auto effkine = truetmp->Rebin(binningpart.size()-1, Form("effKine_R%02d", R), binningpart.data());
        effkine->SetDirectory(nullptr);
        effkine->Divide(effkine, truefull, 1., 1., "b");
        TH1 *truefullall = getFullyEfficientTruth(*mcreader, R, sysvar, binningpart, 0);

        // Get jet finding efficiency
        auto jetFindingEff = static_cast<TH1 *>(truefull->Clone(Form("hJetfindingEfficiency_R%02d", R)));
        jetFindingEff->SetDirectory(nullptr);
        jetFindingEff->Divide(truefullall);


        // split the response matrix for the closure test
        auto responseclosure = getResponseMatrix(*mcreader, R, sysvar, 1);
        responseclosure->SetName(Form("%s_fine", responseclosure->GetName()));
        std::unique_ptr<TH2> truthclosure(getResponseMatrix(*mcreader, R, sysvar, 2));
        truthclosure->SetName("truthclosure");
        responseclosure->Scale(mcscale);
        TH1 *priorsclosuretmp(responseclosure->ProjectionY()),
            *detclosuretmp(truthclosure->ProjectionX()),
            *partclosuretmp(truthclosure->ProjectionY());
        auto priorsclosure = priorsclosuretmp->Rebin(binningpart.size()-1, Form("priorsclosure_R%02d", R), binningpart.data()),
             detclosure = detclosuretmp->Rebin(binningdet.size()-1, Form("detclosure_R%02d", R), binningdet.data()),
             partclosure = partclosuretmp->Rebin(binningpart.size()-1, Form("partclosure_R%02d", R), binningpart.data());
        priorsclosure->SetDirectory(nullptr);
        detclosure->SetDirectory(nullptr);
        partclosure->SetDirectory(nullptr);
        auto rebinnedresponseclosure = makeRebinned2D(responseclosure, binningdet, binningpart);
        rebinnedresponseclosure->SetName(Form("%s_closure", rebinnedresponseclosure->GetName()));
        TH1 *truefullclosure = getFullyEfficientTruth(*mcreader, R, sysvar, binningpart, 1),  // From truth sample
            *jetfindingeffDenom = getFullyEfficientTruth(*mcreader, R, sysvar, binningpart, 2); // from response sample

        // Get jet finding efficiency
        // must be taken from the response sample
        auto jetFindingEffClosure = static_cast<TH1 *>(priorsclosure->Clone(Form("hJetfindingEfficiency_R%02d_closure", R)));
        jetFindingEffClosure->Scale(1/mcscale);
        jetFindingEffClosure->SetDirectory(nullptr);
        jetFindingEffClosure->Divide(jetfindingeffDenom);

        RooUnfoldResponse responsematrix(nullptr, truefull, rebinnedresponse),
                          responsematrixClosure(nullptr, priorsclosure, rebinnedresponseclosure);
        responsematrix.UseOverflow(false);
        responsematrixClosure.UseOverflow(false);

        UnfoldingPool work;
        for(auto ireg : ROOT::TSeqI(1, hraw->GetXaxis()->GetNbins())) {
            work.InsertWork({ireg, radius, hraw, jetFindingEff, &responsematrix, detclosure, &responsematrixClosure, jetFindingEffClosure});
        }

        std::set<unfoldingResults> unfolding_results;
        if(doMT) {
            std::vector<std::thread> workthreads;
            std::mutex combinemutex;
            for(auto i : ROOT::TSeqI(0, 8)){
                workthreads.push_back(std::thread([&combinemutex, &work, &unfolding_results](){
                    UnfoldingRunner worker(&work);
                    worker.DoWork();
                    std::unique_lock<std::mutex> combinelock(combinemutex);
                    for(auto res : worker.getUnfolded()) unfolding_results.insert(res);
                }));
            }
            for(auto &th : workthreads) th.join();
        } else {
            UnfoldingRunner worker(&work);
            worker.DoWork();
            for(auto res : worker.getUnfolded()) unfolding_results.insert(res);
        }

        // Write everything to file
        writer->mkdir(Form("R%02d", R));
        writer->cd(Form("R%02d", R));
        auto basedir = static_cast<TDirectory *>(gDirectory);
        basedir->mkdir("rawlevel");
        basedir->cd("rawlevel");
        mbspectrum.fSpectrum->Write();
        ej1spectrum.fSpectrum->Write();
        ej2spectrum.fSpectrum->Write();
        mbrebinned->Write();
        ej1rebinnedUncorrected->Write();
        ej2rebinnedUncorrected->Write();
        ej1rebinned->Write();
        ej2rebinned->Write();
        trgeffej1->Write();
        trgeffej2->Write();
        rebinnedTriggerEffEJ1->Write();
        rebinnedTriggerEffEJ2->Write();
        hraw->Write();
        auto hnorm = new TH1F("hNorm", "Norm", 1, 0.5, 1.5);
        hnorm->SetBinContent(1, mbspectrum.getNorm());
        hnorm->Write();
        makeEventCounterHistogram("hEventCounterWeighted", "Weighted event counts", ej1spectrum.fEventCount, ej2spectrum.fEventCount, mbspectrum.fEventCount)->Write();
        makeEventCounterHistogram("hEventCounterAbs", "Absolute event counts", ej1spectrum.fEventCountAbsolute, ej2spectrum.fEventCountAbsolute, mbspectrum.fEventCountAbsolute)->Write();
        auto heffVtx = new TH1F("hVertexFindingEfficiency", "Vertex finding efficiency", 1, 0.5, 1.5);
        heffVtx->SetBinContent(1, mbspectrum.fVertexFindingEfficiency);
        heffVtx->Write();
        auto hcntcorr = new TH1F("hCENTNOTRDcorrection", "CENTNOTRD correction", 1, 0.5, 1.5);
        hcntcorr->SetBinContent(1, centnotrdcorrection);
        hcntcorr->Write();
        basedir->mkdir("response");
        basedir->cd("response");
        auto hmcscale = new TH1F("hMCscale", "MC scale", 1, 0.5, 1.5);
        hmcscale->SetBinContent(1, mcscale);
        hmcscale->Write();
        rawresponse->Write();
        rebinnedresponse->Write();
        truefull->Write();
        if(truefullall) truefullall->Write("partall");
        effkine->Write();
        jetFindingEff->Write();
        basedir->mkdir("closuretest");
        basedir->cd("closuretest");
        priorsclosure->Write("priorsclosure");
        detclosure->Write("detclosure");
        partclosure->Write("partclosure");
        if(truefullclosure) truefullclosure->Write("partallclosure");
        responseclosure->Write("responseClosureFine");
        rebinnedresponseclosure->Write("responseClosureRebinned");
        if(jetFindingEffClosure) jetFindingEffClosure->Write();
        for(auto reg : unfolding_results){
            basedir->mkdir(Form("reg%d", reg.fReg));
            basedir->cd(Form("reg%d", reg.fReg));
            if(reg.fUnfolded) reg.fUnfolded->Write();
            if(reg.fNormalized) reg.fNormalized->Write();
            if(reg.fBackfolded) reg.fBackfolded->Write();
            if(reg.fDvector) reg.fDvector->Write();
            if(reg.fPearson) reg.fPearson->Write();
            if(reg.fUnfoldedClosureNoEff) reg.fUnfoldedClosureNoEff->Write();
            if(reg.fUnfoldedClosure) reg.fUnfoldedClosure->Write();
            if(reg.fDvectorClosure) reg.fDvectorClosure->Write();
            if(reg.fPearsonClosure) reg.fPearsonClosure->Write();
        }
    }
}
