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
            const double kSizeEmcalPhi = 1.745,
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

SpectrumAndNorm getClustersAndNorm(TFile &reader, int R, const std::string_view trigger, const std::string_view triggercluster, const std::string_view sysvar) {
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
    auto rawclusters = static_cast<TH1 *>(histos->FindObject("hClusterEnergy1D"));
    rawclusters->SetName(histnamebuilder.str().data());
    rawclusters->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));
    auto norm = hnorm->GetBinContent(clusterbin);
    // Get absolute eventCounts
    auto heventcounts = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));
    auto eventcounts = heventcounts->GetBinContent(clusterbin);
    // calculate vertex finding efficiency
    auto evhist = static_cast<TH1 *>(histos->FindObject("fNormalisationHist")); // take the one from the AliAnalysisTaskEmcal directly
    auto vertexfindingeff = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection"));
    SpectrumAndNorm result{rawclusters, norm, eventcounts, vertexfindingeff};
    return result;
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
    auto hspecandclass = static_cast<TH2 *>(histos->FindObject("hJetSpectrumAbs"));
    auto rawspectrum = hspecandclass->ProjectionY(histnamebuilder.str().data(), clusterbin, clusterbin);
    rawspectrum->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));
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

TH1 *makeEventCounterHistogram(const char *name, const char *title, double countEJE, double countEMC7, double countINT7) {
    auto eventCount = new TH1F(name, title, 3, 0., 3);
    eventCount->GetXaxis()->SetBinLabel(1, "INT7");
    eventCount->GetXaxis()->SetBinLabel(2, "EMC7");
    eventCount->GetXaxis()->SetBinLabel(3, "EJE");
    eventCount->SetBinContent(1, countINT7);
    eventCount->SetBinContent(2, countEMC7);
    eventCount->SetBinContent(3, countEJE);
    return eventCount;
}

std::pair<double, TH1 *> makeRejectionFactor(TH1 *histhigh, TH1 *histlow, int R, const std::string_view triggerlow, const std::string_view triggerhigh, const std::string_view sysvar, double linlow, double linhigh, double eflow, double efhigh, bool fUseErrorFunction = true){
    auto rfac = histcopy(histhigh);
    rfac->SetNameTitle(Form("RejectionFactor_%s_%s_R%02d", triggerhigh.data(), triggerlow.data(), R), Form("Rejection Factor for %s/%s for R=%.1f", triggerhigh.data(), triggerlow.data(), double(R)/10.));
    rfac->SetDirectory(nullptr);
    rfac->Divide(histhigh, histlow, 1., 1.);

    double triggerRF, triggerTF_Error;
    TF1* pol0 = new TF1("pol0", "[0]", linlow, linhigh);
    if(fUseErrorFunction) rfac->Fit(pol0, "QNRMEX0+", "", linlow, linhigh);
    else rfac->Fit(pol0, "QRMEX+", "", linlow, linhigh);
    TF1* erfunc = new TF1("erfunc" ,"[3]+[2] * TMath::Erf( (x-[0])/(TMath::Sqrt(2)*[1]))", eflow, efhigh);

    if(fUseErrorFunction){
        erfunc->SetParameter(0,4.);
        erfunc->SetParameter(1,1.);
        erfunc->SetParameter(2,pol0->GetParameter(0)/2);
        erfunc->SetParameter(3,pol0->GetParameter(0)/2);
        rfac->Fit(erfunc, "QRMEX+", "", eflow, efhigh);
        triggerRF = erfunc->GetParameter(2)+erfunc->GetParameter(3);
        triggerTF_Error = (erfunc->GetParameter(2)+erfunc->GetParameter(3))*TMath::Sqrt(TMath::Power(erfunc->GetParError(2)/erfunc->GetParameter(2),2)+TMath::Power(erfunc->GetParError(3)/erfunc->GetParameter(3),2));
    }else{
        triggerRF = pol0->GetParameter(0);
        triggerTF_Error = pol0->GetParError(0);
    }

    if(fUseErrorFunction) return {triggerRF, rfac};
    else return {triggerRF, rfac};
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

TH1* getFullyEfficientTruth(TFile &reader, int R, const std::string_view sysvar, const std::vector<double> binningpart, bool noclosure) {
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    std::stringstream inputname;
    inputname << "hJetSpectrumPartAll";
    if(noclosure)
        inputname << "NoClosure";
    auto spectrum = static_cast<TH1 *>(histlist->FindObject(inputname.str().data()));
    if(spectrum) {
        std::stringstream outname, outtitle;
        outname << "hTruthFullyEfficient_R" << std::setw(2) << std::setfill('0') << R;
        outtitle << "Fully efficient true spectrum for R=" << std::setprecision(2) << double(R)/10.;
        if(noclosure) {
            outname << "_Closure";
            outtitle << " (for closure test)";
        }
        auto rebinned = spectrum->Rebin(binningpart.size()-1, outname.str().data(), binningpart.data());
        rebinned->SetDirectory(nullptr);
        rebinned->SetTitle(outtitle.str().data());
        return rebinned;
    }
    return nullptr;
}

std::map<std::string, TH1 *> makeJetFindingEffPure(TFile &reader, int R, const std::string_view sysvar, const std::vector<double> binningpart, const std::vector<double> binningdet, bool closure) {
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    std::stringstream inputeffname, inputpurename;
    inputeffname << "hJetfindingEfficiencyCore";
    inputpurename << "hPurityDet";
    if(closure) {
        inputeffname << "Closure";
        inputpurename << "Closure";
    }
    TH2 *raweff = static_cast<TH2 *>(histlist->FindObject(inputeffname.str().data())),
        *rawpure = static_cast<TH2 *>(histlist->FindObject(inputpurename.str().data()));

    if(raweff && rawpure) {
        std::stringstream histnameefficiency, histnamepurity;
        histnameefficiency << "hJetfindingEfficiency_R" << std::setw(2) << std::setfill('0') << R;
        histnameefficiency << "hJetfindingPurity_R" << std::setw(2) << std::setfill('0') << R;
        if(closure) {
            histnameefficiency << "_closure";
            histnamepurity << "_closure";
        }
        std::unique_ptr<TH1> effall(raweff->ProjectionX("effall")),
                             efftag(raweff->ProjectionX("efftag", 3 ,3)),
                             pureall(rawpure->ProjectionX("pureall")),
                             puretag(rawpure->ProjectionX("puretag", 3, 3)),
                             effAllRebinned(effall->Rebin(binningpart.size()-1, "effAllRebin", binningpart.data())),
                             pureAllRebinned(pureall->Rebin(binningdet.size()-1, "pureAllRebin", binningdet.data()));
        auto jetfindingeff = efftag->Rebin(binningpart.size()-1, histnameefficiency.str().data(), binningpart.data()),
             jetfindingpure = puretag->Rebin(binningdet.size()-1, histnamepurity.str().data(), binningdet.data());
        jetfindingeff->SetDirectory(nullptr);
        jetfindingeff->Divide(jetfindingeff, effAllRebinned.get(), 1., 1., "b");
        jetfindingeff->SetTitle(Form("Jet finding efficiency for R = %.1f", double(R)/10.));
        jetfindingpure->SetDirectory(nullptr);
        jetfindingpure->Divide(jetfindingpure, pureAllRebinned.get(), 1., 1., "b");
        jetfindingpure->SetTitle(Form("Jet finding purity for R = %.1f", double(R)/10.));
        return {{"efficiency", jetfindingeff}, {"purity", jetfindingpure}};
    }
    // return empty map
    return std::map<std::string, TH1 *>();
}

TH1 *getDetLevelClosure(TFile &reader, int R, const std::string_view sysvar, const std::vector<double> binning) {
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    std::string inputpurename = "hPurityDetNoClosure";
    TH2 *rawpurity = static_cast<TH2 *>(histlist->FindObject(inputpurename.data())); 
    std::unique_ptr<TH1> detlevelTmp(rawpurity->ProjectionX("pureall"));
    auto detlevel = makeRebinnedSafe(detlevelTmp.get(), "detLevelFull", binning);
    detlevel->SetDirectory(nullptr);
    return detlevel;
}

void runCorrectionChain1DBayes_SpectrumTaskSimplePoor_CorrectEffPure_8TeV(const std::string_view mbfile, const std::string_view emc7file, const std::string_view ejefile, const std::string_view mcfile, const std::string_view sysvar = "", int radiusSel = -1, bool doMT = false){
    ROOT::EnableThreadSafety();
    int NTHREAD=2;
    std::stringstream outputfile;
    outputfile << "correctedBayes_poor_effpure";
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
         binningdet  = getJetPtBinningNonLinSmearPoor(),
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
             trgeffeje    = makeTriggerEfficiency(*mcreader, R, "EJE", sysvar),
             rebinnedTriggerEffEMC7 = makeTriggerEfficiency(*mcreader, R, "EMC7", sysvar, &binningdet),
             rebinnedTriggerEffEJE  = makeTriggerEfficiency(*mcreader, R, "EJE", sysvar, &binningdet);
        auto mbclusters   = getClustersAndNorm(*mbreader, R, "INT7", "ANY", sysvar),
             emc7clusters = getClustersAndNorm(*emc7reader, R, "EMC7", "ANY", sysvar),
             ejeclusters  = getClustersAndNorm(*ejereader, R, "EJE", "ANY", sysvar);

        // Not necessary for 8TeV since trigger cluster "ANY" is used for all triggers
        // apply CENTNOTRD correction
        //ej1spectrum.second->Scale(1./centnotrdcorrection);

        // Uncorrected rebinned Spectra (for monitoring)
        TH1 *mbrebinnedUncorrected = makeRebinnedSafe(mbspectrum.fSpectrum, "mbrebinnedUncorrected", binningdet),
            *emc7rebinnedUncorrected = makeRebinnedSafe(emc7spectrum.fSpectrum, "emc7rebinnedUncorrected", binningdet),
            *ejerebinnedUncorrected  = makeRebinnedSafe(ejespectrum.fSpectrum, "ejerebinnedUncorrected", binningdet);

        // Correct for the trigger efficiency
        emc7spectrum.fSpectrum->Divide(trgeffemc7);
        ejespectrum.fSpectrum->Divide(trgeffeje);

        // Scale by number of events for each trigger
        mbspectrum.fSpectrum->Scale(1/mbspectrum.getNorm());
        emc7spectrum.fSpectrum->Scale(1/emc7spectrum.getNorm());
        ejespectrum.fSpectrum->Scale(1/ejespectrum.getNorm());

        mbclusters.fSpectrum->Scale(1/mbclusters.getNorm());
        emc7clusters.fSpectrum->Scale(1/emc7clusters.getNorm());
        ejeclusters.fSpectrum->Scale(1/ejeclusters.getNorm());

        // Rebin all raw level histograms
        TH1 *mbrebinned(mbspectrum.fSpectrum->Rebin(binningdet.size()-1, "mbrebinned", binningdet.data())),
            *emc7rebinned(emc7spectrum.fSpectrum->Rebin(binningdet.size()-1, "emc7rebinned", binningdet.data())),
            *ejerebinned(ejespectrum.fSpectrum->Rebin(binningdet.size()-1, "ejerebinned", binningdet.data()));

        // Rebin cluster histos for rejection factors
        auto mbrebinned_clusters          = mbclusters.fSpectrum->Rebin(binningrfcourse.size()-1, "mbrebinned_scaled", binningrfcourse.data()),
             emc7rebinned_clusters_course = emc7clusters.fSpectrum->Rebin(binningrfcourse.size()-1, "emc7rebinned_scaled_course", binningrfcourse.data()),
             emc7rebinned_clusters_fine   = emc7clusters.fSpectrum->Rebin(binningrffine.size()-1, "emc7rebinned_scaled_fine", binningrffine.data()),
             ejerebinned_clusters         = ejeclusters.fSpectrum->Rebin(binningrffine.size()-1, "ejerebinned_scaled", binningrffine.data());

        // Scale by bin width after rebin
        mbrebinned_clusters->Scale(1.,"width");
        emc7rebinned_clusters_course->Scale(1.,"width");
        emc7rebinned_clusters_fine->Scale(1.,"width");
        ejerebinned_clusters->Scale(1.,"width");

        // Find the rejection factors
        auto rfactoremc7  = makeRejectionFactor(emc7rebinned_clusters_course, mbrebinned_clusters, R, "INT7", "EMC7", sysvar, 4., 40., 2., 40.),
             rfactoreje   = makeRejectionFactor(ejerebinned_clusters, emc7rebinned_clusters_fine, R, "EMC7", "EJE", sysvar, 12., 60., 6., 200.);

        // Correct for the rejection factors
        emc7spectrum.fSpectrum->Scale(1/rfactoremc7.first);
        ejespectrum.fSpectrum->Scale(1/rfactoremc7.first);
        ejespectrum.fSpectrum->Scale(1/rfactoreje.first);

        emc7rebinned->Scale(1/rfactoremc7.first);
        ejerebinned->Scale(1/rfactoremc7.first);
        ejerebinned->Scale(1/rfactoreje.first);

        // Make combined histograms
        auto hrawOrig = makeCombinedRawSpectrum(*mbrebinned, *emc7rebinned, 50., *ejerebinned, 80.);
        hrawOrig->SetNameTitle(Form("hrawOrig_R%02d", R), Form("Raw Level spectrum R=%.1f, before purity correction", radius));
        hrawOrig->Scale(crosssection);
        auto hrawOrig_fine = makeCombinedRawSpectrum(*mbspectrum.fSpectrum, *emc7spectrum.fSpectrum, 50., *ejespectrum.fSpectrum, 80.);
        hrawOrig_fine->SetNameTitle(Form("hrawOrig_fine_R%02d", R), Form("Raw Level spectrum R=%.1f, fine binning, before purity correction", radius));
        hrawOrig_fine->Scale(crosssection);

        auto effpure = makeJetFindingEffPure(*mcreader, R, sysvar, binningpart, binningdet, false),
             effpureClosure = makeJetFindingEffPure(*mcreader, R, sysvar, binningpart, binningdet, true);

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
        rawresponse->Scale(mcscale);   // undo scaling with the number of trials (used to be 15e6)
        auto rebinnedresponse  = makeRebinned2D(rawresponse, binningdet, binningpart);
        rebinnedresponse->SetName(Form("%s_standard", rebinnedresponse->GetName()));
        std::unique_ptr<TH1> truefulltmp(rawresponse->ProjectionY()),
                             truetmp(rebinnedresponse->ProjectionY());
        auto truefull = truefulltmp->Rebin(binningpart.size() - 1, Form("truefull_R%02d", R), binningpart.data());
        truefull->SetDirectory(nullptr);
        auto effkine = truetmp->Rebin(binningpart.size()-1, Form("effKine_R%02d", R), binningpart.data());
        effkine->SetDirectory(nullptr);
        effkine->Divide(effkine, truefull, 1., 1., "b");
        TH1 *truefullall = getFullyEfficientTruth(*mcreader, R, sysvar, binningpart, false);

        // split the response matrix for the closure test
        auto responseclosure = getResponseMatrix(*mcreader, R, sysvar, 1);
        responseclosure->SetName(Form("%s_fine", responseclosure->GetName()));
        std::unique_ptr<TH2> truthclosure(getResponseMatrix(*mcreader, R, sysvar, 2));
        truthclosure->SetName("truthclosure");
        responseclosure->Scale(mcscale); // Used to be 15e6
        TH1 *priorsclosuretmp(responseclosure->ProjectionY()),
            *partclosuretmp(truthclosure->ProjectionY());
        auto priorsclosure = priorsclosuretmp->Rebin(binningpart.size()-1, Form("priorsclosure_R%02d", R), binningpart.data()),
             partclosure = partclosuretmp->Rebin(binningpart.size()-1, Form("partclosure_R%02d", R), binningpart.data());
        // Det level closure must be taken from purity histogram
        // as the response matrix is already restricted to matched jets
        auto detclosureOrig = getDetLevelClosure(*mcreader, R, sysvar, binningdet);
        auto detclosure = static_cast<TH1 *>(detclosureOrig->Clone(Form("detclosure_R%02d", R)));
        priorsclosure->SetDirectory(nullptr);
        detclosureOrig->SetDirectory(nullptr);
        if(auto closurepure = effpureClosure.find("purity"); closurepure != effpureClosure.end()) {
            detclosure->Multiply(closurepure->second);
        } else {
            detclosure->Multiply(effpure["purity"]);
        }
        partclosure->SetDirectory(nullptr);
        auto rebinnedresponseclosure = makeRebinned2D(responseclosure, binningdet, binningpart);
        rebinnedresponseclosure->SetName(Form("%s_closure", rebinnedresponseclosure->GetName()));
        TH1 *truefullclosure = getFullyEfficientTruth(*mcreader, R, sysvar, binningpart, true);

        RooUnfoldResponse responsematrix(nullptr, truefull, rebinnedresponse),
                          responsematrixClosure(nullptr, priorsclosure, rebinnedresponseclosure);
        responsematrix.UseOverflow(false);
        responsematrixClosure.UseOverflow(false);

        UnfoldingPool work;
        for(auto ireg : ROOT::TSeqI(1, hraw->GetXaxis()->GetNbins())) {
            auto jetFindingEffClosure = effpureClosure.find("efficiency");
            work.InsertWork({ireg, radius, hraw, effpure["efficiency"], &responsematrix, detclosure, &responsematrixClosure, jetFindingEffClosure != effpureClosure.end() ? jetFindingEffClosure->second : nullptr});
        }

        std::set<unfoldingResults> unfolding_results;
        if(doMT) {
            std::vector<std::thread> workthreads;
            std::mutex combinemutex;
            for(auto i : ROOT::TSeqI(0, NTHREAD)){
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
        mbspectrum.fSpectrum->Write();
        emc7spectrum.fSpectrum->Write();
        ejespectrum.fSpectrum->Write();
        mbrebinnedUncorrected->Write();
        emc7rebinnedUncorrected->Write();
        ejerebinnedUncorrected->Write();
        mbrebinned->Write();
        emc7rebinned->Write();
        ejerebinned->Write();
        mbrebinned_clusters->Write();
        emc7rebinned_clusters_course->Write();
        emc7rebinned_clusters_fine->Write();
        ejerebinned_clusters->Write();
        trgeffemc7->Write();
        trgeffeje->Write();
        rebinnedTriggerEffEMC7->Write();
        rebinnedTriggerEffEJE->Write();
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
        hnormINT7->SetBinContent(1, mbspectrum.getNorm());
        hnormINT7->Write();
        hnormEMC7->SetBinContent(1, emc7spectrum.getNorm());
        hnormEMC7->Write();
        hnormEJE->SetBinContent(1, ejespectrum.getNorm());
        hnormEJE->Write();
        makeEventCounterHistogram("hEventCounterWeighted", "Weighted event counts", ejespectrum.fEventCount, emc7spectrum.fEventCount, mbspectrum.fEventCount)->Write();
        makeEventCounterHistogram("hEventCounterAbs", "Absolute event counts", ejespectrum.fEventCountAbsolute, emc7spectrum.fEventCountAbsolute, mbspectrum.fEventCountAbsolute)->Write();
        auto heffVtxINT7 = new TH1F("hVertexFindingEfficiencyINT7", "Vertex finding efficiency, Min Bias Dataset", 1, 0.5, 1.5);
        heffVtxINT7->SetBinContent(1, mbspectrum.fVertexFindingEfficiency);
        heffVtxINT7->Write();
        auto heffVtxEMC7 = new TH1F("hVertexFindingEfficiencyEMC7", "Vertex finding efficiency, EMC7 Dataset", 1, 0.5, 1.5);
        heffVtxEMC7->SetBinContent(1, emc7spectrum.fVertexFindingEfficiency);
        heffVtxEMC7->Write();
        auto heffVtxEJE = new TH1F("hVertexFindingEfficiencyEJE", "Vertex finding efficiency, EJE Dataset", 1, 0.5, 1.5);
        heffVtxEJE->SetBinContent(1, ejespectrum.fVertexFindingEfficiency);
        heffVtxEJE->Write();

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
        if(truefullall) truefullall->Write("partall");
        effkine->Write();
        effpure["efficiency"]->Write();
        basedir->cd();
        basedir->mkdir("closuretest");
        basedir->cd("closuretest");
        priorsclosure->Write("priorsclosure");
        detclosureOrig->Write("detclosureFull");
        detclosure->Write("detclosure");
        partclosure->Write("partclosure");
        if(truefullclosure) truefullclosure->Write("partallclosure");
        responseclosure->Write("responseClosureFine");
        rebinnedresponseclosure->Write("responseClosureRebinned");
        if(auto jetFindingPurityClosure = effpureClosure.find("puriry"); jetFindingPurityClosure != effpureClosure.end()) jetFindingPurityClosure->second->Write();
        if(auto jetFindingEfficiencyClosure = effpureClosure.find("efficiency"); jetFindingEfficiencyClosure != effpureClosure.end()) jetFindingEfficiencyClosure->second->Write();
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
