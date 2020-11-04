#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/roounfold.C"
#include "../../../helpers/math.C"
#include "../../../helpers/root.C"
#include "../../../helpers/unfolding.C"
#include "../../binnings/binningPt1D.C"

struct unfoldingResults {
    int fReg;
    TH1 *fUnfolded;
    TH1 *fNormalized;
    TH1 *fBackfolded;
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
            const double kSizeEmcalPhi = 1.88,
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
            auto specnormalized = static_cast<TH1 *>(specunfolded->Clone(Form("normalizedReg%d", config.fReg)));
            specnormalized->SetNameTitle(Form("normalized_reg%d", config.fReg), Form("Normalized jet spectrum R=%.1f reg %d", config.fRadius, config.fReg));
            specnormalized->SetDirectory(nullptr);
            specnormalized->Scale(1. / (acceptance));
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
            auto specunfoldedClosure = unfolderClosure.Hreco(errorTreatment);
            specunfoldedClosure->SetDirectory(nullptr);
            specunfoldedClosure->SetNameTitle(Form("unfoldedClosure_reg%d", config.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d", config.fRadius, config.fReg));
            specunfoldedClosure->Scale(1., "width");
            TH1 *dvecClosure(nullptr);
            imp = unfolderClosure.Impl();
            if(imp) {
                dvecClosure = histcopy(imp->GetD());
                dvecClosure->SetNameTitle(Form("dvectorClosure_Reg%d", config.fReg), Form("D-vector of the closure test reg %d", config.fReg));
                dvecClosure->SetDirectory(nullptr);
            }
            return {config.fReg, specunfolded, specnormalized, backfolded, specunfoldedClosure, dvec, dvecClosure, 
                    CorrelationHist1D(unfolder.Ereco(), Form("PearsonReg%d", config.fReg), Form("Pearson coefficients regularization %d", config.fReg)),
                    CorrelationHist1D(unfolderClosure.Ereco(), Form("PearsonClosureReg%d", config.fReg), Form("Pearson coefficients of the closure test regularization %d", config.fReg))};
        }

        std::vector<unfoldingResults>   fOutputData;       
        UnfoldingPool                   *fInputData;
};

std::pair<double, TH1 *> getSpectrumAndNorm(TFile &reader, double R, const std::string_view trigger, const std::string_view triggercluster, const std::string_view sysvar) {
    int clusterbin = 0;
    if(triggercluster == "ANY") clusterbin = 1;
    else if(triggercluster == "CENT") clusterbin = 2;
    else if(triggercluster == "CENTNOTRD") clusterbin = 3;
    std::stringstream dirnamebuilder, histnamebuilder;
    dirnamebuilder << "JetSpectrum_FullJets_R" << std::setw(2) << std::setfill('0') << int(R*10.) << "_" << trigger;
    histnamebuilder << "RawJetSpectrum_FullJets_R" <<  std::setw(2) << std::setfill('0') << int(R*10.) << "_" << trigger << "_" << triggercluster;
    if(sysvar.length()) {
        dirnamebuilder << "_" << sysvar;
        histnamebuilder << "_" << sysvar;
    }
    reader.cd(dirnamebuilder.str().data());
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hspecandclass = static_cast<TH2 *>(histos->FindObject("hJetSpectrum"));
    auto rawspectrum = hspecandclass->ProjectionY(histnamebuilder.str().data(), clusterbin, clusterbin);
    rawspectrum->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    auto norm = hnorm->GetBinContent(clusterbin);
    // calculate bin0 correction
    auto evhist = static_cast<TH1 *>(histos->FindObject("fNormalisationHist")); // take the one from the AliAnalysisTaskEmcal directly
    auto bin0correction = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality"));
    norm *= bin0correction;

    return {norm, rawspectrum};
}

TH2 *getResponseMatrix(TFile &reader, double R, const std::string_view sysvar, int closurestatus = 0) {
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
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << int(R*10.) << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto rawresponse = static_cast<TH2 *>(histlist->FindObject(responsematrixname.str().data()));
    rawresponse->SetDirectory(nullptr);
    rawresponse->SetNameTitle(Form("Rawresponse_R%02d", int(R*10.)), Form("Raw response R=%.1f", R));
    return rawresponse;
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

TH1 *makeTriggerEfficiency(TFile &mcreader, double R, const std::string_view trigger, const std::string_view sysvar){
    std::unique_ptr<TH1> mbref(getSpectrumAndNorm(mcreader, R, "INT7", "ANY", sysvar).second),
                         trgspec(getSpectrumAndNorm(mcreader, R, trigger, "ANY", sysvar).second);
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

TH1 *makeUnfoldingWeights(const TH1 *inputCorrected, const TH2 *inputResponsefull) {
    std::unique_ptr<TH1> inputtruefull(inputResponsefull->ProjectionY("truefulltmp"));
    std::unique_ptr<TH1> truefullrebinned(inputtruefull->Rebin(inputCorrected->GetXaxis()->GetNbins(), "truefullrebinned", inputCorrected->GetXaxis()->GetXbins()->GetArray()));
    truefullrebinned->Scale(1., "width");
    TH1 *weighthist = histcopy(inputCorrected);
    weighthist->SetDirectory(nullptr);
    weighthist->SetName("Weights");
    weighthist->Divide(truefullrebinned.get());
    return weighthist;
}

void reweightResponseMatrix(TH2 *responsematrix, const TH1 *weighthist) {
    for(auto partbin : ROOT::TSeqI(0, responsematrix->GetYaxis()->GetNbins())){
        auto weight = weighthist->GetBinContent(partbin + 1);
        for(auto detbin : ROOT::TSeqI(0, responsematrix->GetXaxis()->GetNbins())){
            auto val = responsematrix->GetBinContent(detbin+1, partbin+1) * weight,
                 err = responsematrix->GetBinError(detbin+1, partbin+1) * weight;
            responsematrix->SetBinContent(detbin+1, partbin+1, val);
            responsematrix->SetBinError(detbin+1, partbin+1, val);
        }
    }
}

void reweightPriors(TH1 *priorhist, const TH1 *weighthist) {
    for(auto b : ROOT::TSeqI(0, priorhist->GetXaxis()->GetNbins())){
        auto weight = weighthist->GetBinContent(b+1);
        priorhist->SetBinContent(b+1, priorhist->GetBinContent(b+1) * weight);
        priorhist->SetBinError(b+1, priorhist->GetBinError(b+1) * weight);
    }
}

void runCorrectionChain1DSVD_SpectrumTaskSimpleFineLow_SysPriors(const std::string_view datafile, const std::string_view mcfile, const std::string_view priorsfile, const std::string_view sysvar = ""){
    ROOT::EnableThreadSafety();
    int NTHREAD=2;
    std::stringstream outputfile;
    outputfile << "correctedSVD_fine_lowpt_priors";
    if(sysvar.length()) {
        outputfile << "_" << sysvar;
    }
    outputfile << ".root";
    std::unique_ptr<TFile> datareader(TFile::Open(datafile.data(), "READ")),
                           mcreader(TFile::Open(mcfile.data(), "READ")),
                           priorsreader(TFile::Open(priorsfile.data(), "READ")),
                           writer(TFile::Open(outputfile.str().data(), "RECREATE"));
    auto binningpart = getJetPtBinningNonLinTrueLargeFineLow(),
         binningdet = getJetPtBinningNonLinSmearLargeFineLow();
    auto centnotrdcorrection = getCENTNOTRDCorrection(*datareader, sysvar);
    double crosssection = 57.8;
    for(double radius = 0.2; radius <= 0.6; radius += 0.1) {
        std::cout << "Doing jet radius " << radius << std::endl;
        auto mbspectrum = getSpectrumAndNorm(*datareader, radius, "INT7", "ANY", sysvar),
             ej1spectrum = getSpectrumAndNorm(*datareader, radius, "EJ1", "CENTNOTRD", sysvar),
             ej2spectrum = getSpectrumAndNorm(*datareader, radius, "EJ2", "CENT", sysvar);
        auto trgeffej1 = makeTriggerEfficiency(*mcreader, radius, "EJ1", sysvar),
             trgeffej2 = makeTriggerEfficiency(*mcreader, radius, "EJ2", sysvar);

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
        auto rawresponse = getResponseMatrix(*mcreader, radius, sysvar);
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
        auto responseclosure = getResponseMatrix(*mcreader, radius, sysvar, 1);
        responseclosure->SetName(Form("%s_fine", responseclosure->GetName()));
        std::unique_ptr<TH2> truthclosure(getResponseMatrix(*mcreader, radius, sysvar, 2));
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

        // get the priors weights
        priorsreader->cd(Form("R%02d", int(radius * 10)));
        auto priorsbase = gDirectory;
        priorsbase->cd("response");
        auto priorsresponse = static_cast<TH2 *>(gDirectory->Get(Form("Rawresponse_R%02d_fine", int(radius * 10.))));
        priorsbase->cd("reg6");
        auto priorsdata = static_cast<TH1 *>(gDirectory->Get("unfolded_reg6"));
        auto priorsweight = makeUnfoldingWeights(priorsdata, priorsresponse);

        // reweight response matrix and priors
        reweightResponseMatrix(rebinnedresponse, priorsweight);
        reweightResponseMatrix(rebinnedresponseclosure, priorsweight);
        reweightPriors(truefull, priorsweight);
        reweightPriors(priorsclosure, priorsweight);

        RooUnfoldResponse responsematrix(nullptr, truefull, rebinnedresponse),
                          responsematrixClosure(nullptr, priorsclosure, rebinnedresponseclosure);
        responsematrix.UseOverflow(false);
        responsematrixClosure.UseOverflow(false);

        UnfoldingPool work;
        for(auto ireg : ROOT::TSeqI(2, 10)) {
            work.InsertWork({ireg, radius, hraw, &responsematrix, detclosure, &responsematrixClosure});
        }

        std::vector<std::thread> workthreads;
        std::set<unfoldingResults> unfolding_results;
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

        // Write everything to file
        writer->mkdir(Form("R%02d", int(radius*10)));
        writer->cd(Form("R%02d", int(radius*10)));
        auto basedir = gDirectory;
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
        priorsweight->Write();
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
            if(reg.fDvector) reg.fDvector->Write();
            if(reg.fPearson) reg.fPearson->Write();
            if(reg.fUnfoldedClosure) reg.fUnfoldedClosure->Write();
            if(reg.fDvectorClosure) reg.fDvectorClosure->Write();
            if(reg.fPearsonClosure) reg.fPearsonClosure->Write();
        }
    }
}
