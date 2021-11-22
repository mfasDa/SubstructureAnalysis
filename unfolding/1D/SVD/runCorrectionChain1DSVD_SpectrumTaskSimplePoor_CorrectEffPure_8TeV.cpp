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

struct unfoldingResults {
    int fReg;
    TH1 *fUnfolded;
    TH1 *fNormalizedNoEff;
    TH1 *fNormalized;
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
            const double kSizeEmcalPhi = 1.745,
                         kSizeEmcalEta = 1.4;
            auto acceptance = (kSizeEmcalPhi - 2 * config.fRadius) * (kSizeEmcalEta - 2 * config.fRadius) / (TMath::TwoPi());
            std::cout << "[SVD unfolding] Regularization " << config.fReg << "\n================================================================\n";
            std::cout << "[SVD unfolding] Running unfolding" << std::endl;
            RooUnfoldSvd unfolder(config.fResponseMatrix, config.fRaw, config.fReg);
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
            TH1 *dvec(nullptr);
            auto imp = unfolder.Impl();
            if(imp){
                dvec = histcopy(imp->GetD());
                dvec->SetNameTitle(Form("dvector_Reg%d", config.fReg), Form("D-vector reg %d", config.fReg));
                dvec->SetDirectory(nullptr);
            }

            // run closure test
            std::cout << "[SVD unfolding] Running closure test" << std::endl;
            RooUnfoldSvd unfolderClosure(config.fResponseMatrixClosure, config.fDetLevelClosure, config.fReg);
            auto specunfoldedClosureNoEff = unfolderClosure.Hreco(errorTreatment);
            specunfoldedClosureNoEff->SetDirectory(nullptr);
            specunfoldedClosureNoEff->SetNameTitle(Form("unfoldedClosureNoEff_reg%d", config.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d, no correction for jet finding efficiency", config.fRadius, config.fReg));
            specunfoldedClosureNoEff->Scale(1., "width");
            auto specunfoldedClosure = static_cast<TH1 *>(specunfoldedClosureNoEff->Clone());
            specunfoldedClosure->SetDirectory(nullptr);
            specunfoldedClosure->SetNameTitle(Form("unfoldedClosure_reg%d", config.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d", config.fRadius, config.fReg));
            specunfoldedClosure->Divide(config.fJetFindingEff);
            TH1 *dvecClosure(nullptr);
            imp = unfolderClosure.Impl();
            if(imp) {
                dvecClosure = histcopy(imp->GetD());
                dvecClosure->SetNameTitle(Form("dvectorClosure_Reg%d", config.fReg), Form("D-vector of the closure test reg %d", config.fReg));
                dvecClosure->SetDirectory(nullptr);
            }
            return {config.fReg, specunfolded, specnormalizedNoEff, specnormalized, backfolded, specunfoldedClosureNoEff, specunfoldedClosure, dvec, dvecClosure,
                    CorrelationHist1D(unfolder.Ereco(), Form("PearsonReg%d", config.fReg), Form("Pearson coefficients regularization %d", config.fReg)),
                    CorrelationHist1D(unfolderClosure.Ereco(), Form("PearsonClosureReg%d", config.fReg), Form("Pearson coefficients of the closure test regularization %d", config.fReg))};
        }

        std::vector<unfoldingResults>   fOutputData;
        UnfoldingPool                   *fInputData;
};

std::pair<double, TH1 *> getClustersAndNorm(TFile &reader, int R, const std::string_view trigger, const std::string_view triggercluster, const std::string_view sysvar) {
    int clusterbin = 0;
    if(triggercluster == "ANY") clusterbin = 1;
    else if(triggercluster == "CENT") clusterbin = 2;
    else if(triggercluster == "CENTNOTRD") clusterbin = 3;
    std::stringstream dirnamebuilder, histnamebuilder;
    dirnamebuilder << "JetSpectrum_FullJets_R" << std::setw(2) << std::setfill('0') << R << "_" << trigger;
    histnamebuilder << "Clusters_FullJets_R" <<  std::setw(2) << std::setfill('0') << R << "_" << trigger;
    if(sysvar.length()) {
        dirnamebuilder << "_" << sysvar;
        histnamebuilder << "_" << sysvar;
    }
    reader.cd(dirnamebuilder.str().data());
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hclusandclass = static_cast<TH2 *>(histos->FindObject("hQAClusterTimeVsEFine"));
    auto rawclusters = hclusandclass->ProjectionY(histnamebuilder.str().data(), 1, hclusandclass->GetNbinsX());
    rawclusters->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));
    auto norm = hnorm->GetBinContent(clusterbin);
    // calculate bin0 correction
    auto evhist = static_cast<TH1 *>(histos->FindObject("fNormalisationHist")); // take the one from the AliAnalysisTaskEmcal directly
    auto bin0correction = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality"));
    norm *= bin0correction;

    return {norm, rawclusters};
}

std::pair<double, TH1 *> getSpectrumAndNorm(TFile &reader, int R, const std::string_view trigger, const std::string_view triggercluster, const std::string_view sysvar) {
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
    auto hspecandclass = static_cast<TH2 *>(histos->FindObject("hJetSpectrumAbs"));
    auto rawspectrum = hspecandclass->ProjectionY(histnamebuilder.str().data(), clusterbin, clusterbin);
    rawspectrum->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));
    auto norm = hnorm->GetBinContent(clusterbin);
    // calculate bin0 correction
    auto evhist = static_cast<TH1 *>(histos->FindObject("fNormalisationHist")); // take the one from the AliAnalysisTaskEmcal directly
    auto bin0correction = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality"));
    norm *= bin0correction;

    return {norm, rawspectrum};
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
    dirnamebuilder << "JetSpectrum_FullJets_R02_EJE";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    return hnorm->GetBinContent(3)/hnorm->GetBinContent(2);
}

std::pair<double, TH1 *> makeRejectionFactor(TH1 *histhigh, TH1 *histlow, int R, const std::string_view triggerlow, const std::string_view triggerhigh, const std::string_view sysvar, double triggerswap){
    auto rfac = histcopy(histhigh);
    rfac->SetNameTitle(Form("RejecionFactor_%s/%s_R%02d", triggerhigh.data(), triggerlow.data(), R), Form("Rejection Factor for %s/%s for R=%.1f", triggerhigh.data(), triggerlow.data(), double(R)/10.));
    rfac->SetDirectory(nullptr);
    rfac->Divide(histhigh, histlow, 1., 1.);

    rfac->Fit("pol0","EQ","Same",triggerswap,rfac->GetMaximum());
    auto fitResult = rfac->GetFunction("pol0");
    auto fit = fitResult->GetParameter(0);
    auto fitError = fitResult->GetParError(0);
    fitResult->SetLineColor(kBlue+2);
    return {fit, rfac};
}

TH1 *makeTriggerEfficiency(TFile &mcreader, int R, const std::string_view trigger, const std::string_view sysvar){
    std::unique_ptr<TH1> mbref(getSpectrumAndNorm(mcreader, R, "INT7", "ANY", sysvar).second),
                         trgspec(getSpectrumAndNorm(mcreader, R, trigger, "ANY", sysvar).second);
    auto eff = histcopy(trgspec.get());
    eff->SetNameTitle(Form("TriggerEfficiency_%s_R%02d", trigger.data(), R), Form("Trigger efficiency for %s for R=%.1f", trigger.data(), double(R)/10.));
    eff->SetDirectory(nullptr);
    eff->Divide(trgspec.get(), mbref.get(), 1., 1., "b");
    eff->Fit("pol0","EQ","Same",250,350);
    auto fitResult = eff->GetFunction("pol0");
    auto fit = fitResult->GetParameter(0);
    auto fitError = fitResult->GetParError(0);
    fitResult->SetLineColor(kBlue+2);
    eff->Scale(1./fit);
    return eff;
}

TH1 *makeCombinedRawSpectrum(const TH1 &mb, const TH1 &emc7, double emc7swap, const TH1 &eje, double ejeswap){
    auto combined = histcopy(&mb);
    combined->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(combined->GetXaxis()->FindBin(emc7swap), combined->GetXaxis()->FindBin(ejeswap))) {
        std::cout << "[" << combined->GetXaxis()->GetBinLowEdge(b) << " - " << combined->GetXaxis()->GetBinUpEdge(b) << "] Using EMC7" << std::endl;
        combined->SetBinContent(b, emc7.GetBinContent(b));
        combined->SetBinError(b, emc7.GetBinError(b));
    }
    for(auto b : ROOT::TSeqI(combined->GetXaxis()->FindBin(ejeswap), combined->GetXaxis()->GetNbins()+1)) {
        std::cout << "[" << combined->GetXaxis()->GetBinLowEdge(b) << " - " << combined->GetXaxis()->GetBinUpEdge(b) << "] Using EJE" << std::endl;
        combined->SetBinContent(b, eje.GetBinContent(b));
        combined->SetBinError(b, eje.GetBinError(b));
    }
    return combined;
}

std::map<std::string, TH1 *> makeJetFindingEffPure(TFile &reader, int R, std::string_view sysvar, std::vector<double> binningpart, std::vector<double> binningdet) {
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto raweff = static_cast<TH2 *>(histlist->FindObject("hJetfindingEfficiencyCore")),
         rawpure = static_cast<TH2 *>(histlist->FindObject("hPurityDet"));

    std::unique_ptr<TH1> effall(raweff->ProjectionX("effall")),
                         efftag(raweff->ProjectionX("efftag", 3 ,3)),
                         pureall(rawpure->ProjectionX("pureall")),
                         puretag(rawpure->ProjectionX("puretag", 3, 3)),
                         effAllRebinned(effall->Rebin(binningpart.size()-1, "effAllRebin", binningpart.data())),
                         pureAllRebinned(pureall->Rebin(binningdet.size()-1, "pureAllRebin", binningdet.data()));
    auto jetfindingeff = efftag->Rebin(binningpart.size()-1, Form("hJetfindingEfficiency_R%02d", R), binningpart.data()),
         jetfindingpure = puretag->Rebin(binningdet.size()-1, Form("hJetfindingPurity_R%02d", R), binningdet.data());
    jetfindingeff->SetDirectory(nullptr);
    jetfindingeff->Divide(jetfindingeff, effAllRebinned.get(), 1., 1., "b");
    jetfindingeff->SetTitle(Form("Jet finding efficiency for R = %.1f", double(R)/10.));
    jetfindingpure->SetDirectory(nullptr);
    jetfindingpure->Divide(jetfindingpure, pureAllRebinned.get(), 1., 1., "b");
    jetfindingpure->SetTitle(Form("Jet finding purity for R = %.1f", double(R)/10.));
    return {{"efficiency", jetfindingeff}, {"purity", jetfindingpure}};
}


void runCorrectionChain1DSVD_SpectrumTaskSimplePoor_CorrectEffPure_8TeV(const std::string_view mbfile, const std::string_view emc7file, const std::string_view ejefile, const std::string_view mcfile, const std::string_view sysvar = "", int radiusSel = -1, bool doMT = false){
    ROOT::EnableThreadSafety();
    std::stringstream outputfile;
    outputfile << "correctedSVD_poor_effpure";
    if(sysvar.length()) {
        outputfile << "_" << sysvar;
    }
    if(radiusSel > 0) {
        outputfile << "_R" << std::setfill('0') << std::setw(2) << radiusSel;
    }
    outputfile << ".root";
    std::unique_ptr<TFile> mbreader(TFile::Open(mbfile.data(), "READ")),
                           emc7reader(TFile::Open(emc7file.data(), "READ")),
                           ejereader(TFile::Open(ejefile.data(), "READ")),
                           mcreader(TFile::Open(mcfile.data(), "READ")),
                           writer(TFile::Open(outputfile.str().data(), "RECREATE"));

    auto binningpart = getJetPtBinningNonLinTruePoor(),
         binningdet = getJetPtBinningNonLinSmearPoor(),
         binningrffine   = getJetPtBinningRejectionFactorsFine(),
         binningrfcourse = getJetPtBinningRejectionFactorsCourse();

    // Not necessary for 8TeV since trigger cluster "ANY" is used for all triggers
    //auto centnotrdcorrection = getCENTNOTRDCorrection(*datareader, sysvar);

    double crosssection = 55.8;

    for(auto R : ROOT::TSeqI(2, 7)) {
        double radius = double(R) / 10.;
        if(radiusSel > 0 && R != radiusSel) {
            continue;
        }
        std::cout << "Doing jet radius " << radius << std::endl;
        auto mbspectrum   = getSpectrumAndNorm(*mbreader, R, "INT7", "ANY", sysvar),
             emc7spectrum = getSpectrumAndNorm(*emc7reader, R, "EMC7", "ANY", sysvar),
             ejespectrum  = getSpectrumAndNorm(*ejereader, R, "EJE", "ANY", sysvar);
        auto trgeffemc7   = makeTriggerEfficiency(*mcreader, R, "EMC7", sysvar),
             trgeffeje    = makeTriggerEfficiency(*mcreader, R, "EJE", sysvar);
        auto mbclusters   = getClustersAndNorm(*mbreader, R, "INT7", "ANY", sysvar),
             emc7clusters = getClustersAndNorm(*emc7reader, R, "EMC7", "ANY", sysvar),
             ejeclusters  = getClustersAndNorm(*ejereader, R, "EJE", "ANY", sysvar);

        // Not necessary for 8TeV since trigger cluster "ANY" is used for all triggers
        // apply CENTNOTRD correction
        //ej1spectrum.second->Scale(1./centnotrdcorrection);

        // Correct for the trigger efficiency
        emc7spectrum.second->Divide(trgeffemc7);
        ejespectrum.second->Divide(trgeffeje);

        // Scale by number of events for each trigger
        mbspectrum.second->Scale(1/mbspectrum.first);
        emc7spectrum.second->Scale(1/emc7spectrum.first);
        ejespectrum.second->Scale(1/ejespectrum.first);

        mbclusters.second->Scale(1/mbclusters.first);
        emc7clusters.second->Scale(1/emc7clusters.first);
        ejeclusters.second->Scale(1/ejeclusters.first);

        // Rebin all raw level histograms
        std::unique_ptr<TH1> mbrebinned(mbspectrum.second->Rebin(binningdet.size()-1, "mbrebinned", binningdet.data())),
                             emc7rebinned(emc7spectrum.second->Rebin(binningdet.size()-1, "emc7rebinned", binningdet.data())),
                             ejerebinned(ejespectrum.second->Rebin(binningdet.size()-1, "ejerebinned", binningdet.data()));

        // Rebin cluster histos for rejection factors
        auto mbrebinned_clusters          = mbclusters.second->Rebin(binningrfcourse.size()-1, "mbrebinned_scaled", binningrfcourse.data()),
             emc7rebinned_clusters_course = emc7clusters.second->Rebin(binningrfcourse.size()-1, "emc7rebinned_scaled_course", binningrfcourse.data()),
             emc7rebinned_clusters_fine   = emc7clusters.second->Rebin(binningrffine.size()-1, "emc7rebinned_scaled_fine", binningrffine.data()),
             ejerebinned_clusters         = ejeclusters.second->Rebin(binningrffine.size()-1, "ejerebinned_scaled", binningrffine.data());

        // Scale by bin width after rebin
        mbrebinned_clusters->Scale(1.,"width");
        emc7rebinned_clusters_course->Scale(1.,"width");
        emc7rebinned_clusters_fine->Scale(1.,"width");
        ejerebinned_clusters->Scale(1.,"width");

        // Find the rejection factors
        auto rfactoremc7  = makeRejectionFactor(emc7rebinned_clusters_course, mbrebinned_clusters, R, "INT7", "EMC7", sysvar, 5.),
             rfactoreje   = makeRejectionFactor(ejerebinned_clusters, emc7rebinned_clusters_fine, R, "EMC7", "EJE", sysvar, 12.);

        // Correct for the rejection factors
        emc7spectrum.second->Scale(1/rfactoremc7.first);
        ejespectrum.second->Scale(1/rfactoremc7.first);
        ejespectrum.second->Scale(1/rfactoreje.first);

        emc7rebinned->Scale(1/rfactoremc7.first);
        ejerebinned->Scale(1/rfactoremc7.first);
        ejerebinned->Scale(1/rfactoreje.first);

        // Make combined histograms
        auto hrawOrig = makeCombinedRawSpectrum(*mbrebinned, *emc7rebinned, 50., *ejerebinned, 80.);
        hrawOrig->SetNameTitle(Form("hrawOrig_R%02d", R), Form("Raw Level spectrum R=%.1f, before purity correction", radius));
        hrawOrig->Scale(crosssection);
        auto hrawOrig_fine = makeCombinedRawSpectrum(*mbspectrum.second, *emc7spectrum.second, 50., *ejespectrum.second, 80.);
        hrawOrig_fine->SetNameTitle(Form("hrawOrig_fine_R%02d", R), Form("Raw Level spectrum R=%.1f, fine binning, before purity correction", radius));
        hrawOrig_fine->Scale(crosssection);

        auto effpure = makeJetFindingEffPure(*mcreader, R, sysvar, binningpart, binningdet);

        auto hraw = static_cast<TH1 *>(hrawOrig->Clone());
        hraw->SetNameTitle(Form("hraw_R%02d", R), Form("Raw Level spectrum R=%.1f", radius));
        hraw->Multiply(effpure["purity"]);
        auto hraw_fine = static_cast<TH1 *>(hrawOrig_fine->Clone());
        hraw_fine->SetNameTitle(Form("hraw_fine_R%02d", R), Form("Raw Level spectrum (fine) R=%.1f, no purity correction", radius));
        //hraw_fine->Multiply(effpure["purity"]);

        // Get the response matrix
        auto rawresponse = getResponseMatrix(*mcreader, R, sysvar);
        auto trials = getTrials(*mcreader, R, sysvar);
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
             detclosureOrig = detclosuretmp->Rebin(binningdet.size()-1, Form("detclosureOrig_R%02d", R), binningdet.data()),
             partclosure = partclosuretmp->Rebin(binningpart.size()-1, Form("partclosure_R%02d", R), binningpart.data());
        auto detclosure = static_cast<TH1 *>(detclosureOrig->Clone(Form("detclosure_R%02d", R)));
        priorsclosure->SetDirectory(nullptr);
        detclosureOrig->SetDirectory(nullptr);
        detclosure->Multiply(effpure["purity"]);
        partclosure->SetDirectory(nullptr);
        auto rebinnedresponseclosure = makeRebinned2D(responseclosure, binningdet, binningpart);
        rebinnedresponseclosure->SetName(Form("%s_closure", rebinnedresponseclosure->GetName()));

        RooUnfoldResponse responsematrix(nullptr, truefull, rebinnedresponse),
                          responsematrixClosure(nullptr, priorsclosure, rebinnedresponseclosure);
        responsematrix.UseOverflow(false);
        responsematrixClosure.UseOverflow(false);

        UnfoldingPool work;
        for(auto ireg : ROOT::TSeqI(1, hraw->GetXaxis()->GetNbins())) {
            work.InsertWork({ireg, radius, hraw, effpure["efficiency"], &responsematrix, detclosure, &responsematrixClosure});
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
        };

        // Write everything to file
        writer->mkdir(Form("R%02d", R));
        writer->cd(Form("R%02d", R));
        auto basedir = static_cast<TDirectory *>(gDirectory);
        basedir->mkdir("rawlevel");
        basedir->cd("rawlevel");
        mbspectrum.second->Write();
        ejespectrum.second->Write();
        emc7spectrum.second->Write();
        mbrebinned_clusters->Write();
        emc7rebinned_clusters_course->Write();
        emc7rebinned_clusters_fine->Write();
        ejerebinned_clusters->Write();
        trgeffeje->Write();
        trgeffemc7->Write();
        rfactoremc7.second->Write();
        rfactoreje.second->Write();
        effpure["purity"]->Write();
        hrawOrig->Write();
        hraw->Write();
        hrawOrig_fine->Write();
        hraw_fine->Write();
        auto hnormINT7 = new TH1F("hNorm_INT7", "Norm - INT7", 1, 0.5, 1.5);
        auto hnormEMC7 = new TH1F("hNorm_EMC7", "Norm - EMC7", 1, 0.5, 1.5);
        auto hnormEJE = new TH1F("hNorm_EJE", "Norm - EJE", 1, 0.5, 1.5);
        hnormINT7->SetBinContent(1, mbspectrum.first);
        hnormINT7->Write();
        hnormEMC7->SetBinContent(1, emc7spectrum.first);
        hnormEMC7->Write();
        hnormEJE->SetBinContent(1, ejespectrum.first);
        hnormEJE->Write();

        // Not necessary for 8TeV since trigger cluster "ANY" is used for all triggers
        //auto hcntcorr = new TH1F("hCENTNOTRDcorrection", "CENTNOTRD correction", 1, 0.5, 1.5);
        //hcntcorr->SetBinContent(1, centnotrdcorrection);
        //hcntcorr->Write();

        basedir->cd();
        basedir->mkdir("response");
        basedir->cd("response");
        auto hmcscale = new TH1F("hMCscale", "MC scale", 1, 0.5, 1.5);
        hmcscale->SetBinContent(1, mcscale);
        hmcscale->Write();
        rawresponse->Write();
        rebinnedresponse->Write();
        truefull->Write();
        effkine->Write();
        effpure["efficiency"]->Write();
        basedir->cd();
        basedir->mkdir("closuretest");
        basedir->cd("closuretest");
        priorsclosure->Write("priorsclosure");
        detclosure->Write("detclosure");
        partclosure->Write("partclosure");
        responseclosure->Write("responseClosureFine");
        rebinnedresponseclosure->Write("responseClosureRebinned");
        for(auto reg : unfolding_results){
            basedir->cd();
            basedir->mkdir(Form("reg%d", reg.fReg));
            basedir->cd(Form("reg%d", reg.fReg));
            if(reg.fUnfolded) reg.fUnfolded->Write();
            if(reg.fNormalizedNoEff) reg.fNormalizedNoEff->Write();
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
