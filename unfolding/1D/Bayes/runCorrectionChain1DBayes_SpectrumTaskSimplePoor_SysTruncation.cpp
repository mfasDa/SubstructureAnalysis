#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/roounfold.C"
#include "../../../helpers/math.C"
#include "../../../helpers/unfolding.C"
#include "../../binnings/binningPt1D.C"
#include "../../binnings/binningPt1D_truncation.C"

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
            auto specnormalized = static_cast<TH1 *>(specunfolded->Clone(Form("normalizedReg%d", config.fReg)));
            specnormalized->SetNameTitle(Form("normalized_reg%d", config.fReg), Form("Normalized jet spectrum R=%.1f reg %d", config.fRadius, config.fReg));
            specnormalized->SetDirectory(nullptr);
            specnormalized->Scale(1. / (acceptance));

            // run closure test
            std::cout << "[Bayes unfolding] Running closure test" << std::endl;
            RooUnfoldBayes unfolderClosure(config.fResponseMatrixClosure, config.fDetLevelClosure, config.fReg);
            auto specunfoldedClosure = unfolderClosure.Hreco(errorTreatment);
            specunfoldedClosure->SetDirectory(nullptr);
            specunfoldedClosure->SetNameTitle(Form("unfoldedClosure_reg%d", config.fReg), Form("Unfolded jet spectrum of the closure test R=%.1f reg %d", config.fRadius, config.fReg));
            specunfoldedClosure->Scale(1., "width");
            return {config.fReg, specunfolded, specnormalized, backfolded, specunfoldedClosure, nullptr, nullptr, 
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


void runCorrectionChain1DBayes_SpectrumTaskSimplePoor_SysTruncation(const std::string_view datafile, const std::string_view mcfile, const std::string_view truncvar, const std::string_view sysvar = ""){
    ROOT::EnableThreadSafety();
    int NTHREAD=2;
    std::stringstream outputfile;
    outputfile << "correctedBayes_poor_truncation" << truncvar;
    if(sysvar.length()) {
        outputfile << "_" << sysvar;
    }
    outputfile << ".root";
    std::unique_ptr<TFile> datareader(TFile::Open(datafile.data(), "READ")),
                           mcreader(TFile::Open(mcfile.data(), "READ")),
                           writer(TFile::Open(outputfile.str().data(), "RECREATE"));
    auto binningpart = getJetPtBinningNonLinTruePoor(),
         binningdet = getJetPtBinningNonLinSmearPoorTrunc(truncvar);
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
                             ej1rebinned(ej1spectrum.second->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data())),
                             ej2rebinned(ej2spectrum.second->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data()));
        auto hraw = makeCombinedRawSpectrum(*mbrebinned, *ej2rebinned, 50., *ej1rebinned, 100.);
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

        RooUnfoldResponse responsematrix(nullptr, truefull, rebinnedresponse),
                          responsematrixClosure(nullptr, priorsclosure, rebinnedresponseclosure);
        responsematrix.UseOverflow(false);
        responsematrixClosure.UseOverflow(false);

        UnfoldingPool work;
        for(auto ireg : ROOT::TSeqI(1, hraw->GetXaxis()->GetNbins())) {
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
            if(reg.fDvector) reg.fDvector->Write();
            if(reg.fPearson) reg.fPearson->Write();
            if(reg.fUnfoldedClosure) reg.fUnfoldedClosure->Write();
            if(reg.fDvectorClosure) reg.fDvectorClosure->Write();
            if(reg.fPearsonClosure) reg.fPearsonClosure->Write();
        }
    }
}
