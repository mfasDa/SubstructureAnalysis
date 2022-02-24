#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/roounfold.C"

#include "../../../helpers/math.C"
#include "../../../helpers/unfolding.C"

#include "../../../struct/spectrum1D/DataFileHandler.cxx"
#include "../../../struct/spectrum1D/LuminosityHistograms.cxx"
#include "../../../struct/spectrum1D/MCFileHandler.cxx"
#include "../../../struct/spectrum1D/OutputHandler.cxx"
#include "../../../struct/spectrum1D/ResponseHandler.cxx"
#include "../../../struct/spectrum1D/TriggerEfficiency.cxx"
#include "../../../struct/spectrum1D/UnfoldingHandler.cxx"

#include "../../binnings/binningPt1D.C"

TH1 *makeRebinnedSafe(const TH1 *input, const char *newname, const std::vector<double> binning) {
    std::unique_ptr<TH1> rebinhelper(histcopy(input));
    auto rebinned = rebinhelper->Rebin(binning.size()-1, newname, binning.data());
    return rebinned;
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

void runNewCorrectionChain1DSVD_SpectrumTaskSimplePoor_CorrectEffPure_EffLumi(const std::string_view file2017 = "", const std::string_view file2018 = "", const std::string_view filemc = "", const std::string_view sysvar = "", int radiusSel = -1, bool doMT = false) {
    ROOT::EnableThreadSafety();
    const std::string jettype = "FullJets";
    std::array<std::string, 3> TRIGGERS = {{"INT7", "EJ2", "EJ1"}};
    UnfoldingHandler::UnfoldingMethod_t unfoldingmethod = UnfoldingHandler::UnfoldingMethod_t::kSVD;
    std::map<int, std::shared_ptr<DataFileHandler>> mDataFileHandlers;
    auto lumihists = std::make_shared<LuminosityHistograms>();
    if(file2017.length() && file2017 != "None") {
        std::cout << "Reading file for 2017: " << file2017 << std::endl;
        auto filehandler = std::make_shared<DataFileHandler>(file2017, jettype, sysvar);
        lumihists->addYear(2017, filehandler->getLuminosityHandler());
        mDataFileHandlers[2017] = filehandler;
    }
    if(file2018.length() && file2018 != "None") {
        std::cout << "Reading file for 2018: " << file2018 << std::endl;
        auto filehandler = std::make_shared<DataFileHandler>(file2018, jettype, sysvar);
        lumihists->addYear(2018, filehandler->getLuminosityHandler());
        mDataFileHandlers[2018] = filehandler;
    }
    lumihists->build();
    MCFileHandler mchandler(filemc, jettype,  sysvar);
    OutputHandler outhandler;

    std::vector<double> binningdet = getJetPtBinningNonLinSmearPoor(),
                        binningpart = getJetPtBinningNonLinTruePoor();
    const PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t lumiunit =  PWG::EMCAL::AliEmcalTriggerLuminosity::LuminosityUnit_t::kMb;
    for(auto R : ROOT::TSeqI(2, 7)) {
        double radius = double(R) / 10.;
        if(radiusSel > 0 && R != radiusSel) {
            continue;
        }
        std::cout << "Doing jet radius " << radius << std::endl;
        // Set the luminosity histograms
        outhandler.setLuminosityHistograms(R, lumihists);

        // Adding spectra for each trigger class from the different years
        // add also integrated luminosity, scaled by the corresponding vertex finding efficiencies
        std::map<std::string, TH1 *> rawspectrumTrigger;
        std::map<std::string, double> norms = {{"INT7", 0.}, {"EJ2", 0.}, {"EJ1", 0.}};
        for(auto &[year, filehandler] : mDataFileHandlers) {
            auto effVtx = filehandler->getVertexFindingEfficiency(R, "INT7").evaluate();
            outhandler.setVertexFindingEfficiency(R, year, effVtx);
            outhandler.setCENTNOTRDCorrection(R, year, filehandler->getClusterCounterAbs(R, "EJ1").makeClusterRatio(TriggerCluster::kCENTNOTRD, TriggerCluster::kCENT));
            auto lumihandler = filehandler->getLuminosityHandler();
            for(auto &trg : TRIGGERS) {
                auto spec =  filehandler->getSpectrumAbs(R, trg).getSpectrumForTriggerCluster(TriggerCluster::kANY);
                auto trgfound = rawspectrumTrigger.find(trg);
                if(trgfound != rawspectrumTrigger.end()) trgfound->second->Add(spec);
                else rawspectrumTrigger[trg] = spec;
                // luminosity from Luminosity handler does not include vertex finding efficiency, which needs to be evaluated by the user
                // use effective luminosity for EMCAL triggers using observed downscaling instead of expected downscaling
                auto rawlumi = (trg == "INT7") ? lumihandler->GetLuminosityForTrigger(trg.data(),lumiunit) : lumihandler->GetEffectiveLuminosityForTrigger(trg.data(), lumiunit);
                norms[trg] += rawlumi / effVtx;
                outhandler.setRawEvents(R, year, trg, filehandler->getClusterCounterAbs(R, trg).getCounters(TriggerCluster::kANY));
            }
        }

        auto &mcset = mchandler.getMCSet(R);
        auto &closureset = mchandler.getClosureSet(R);

        std::unordered_map<std::string, TriggerEfficiency> triggerEffs = {
            {"EJ2", TriggerEfficiency(mcset.getDetLevelSpectrum(MCFileHandler::TriggerClass::INT7), 
                                      mcset.getDetLevelSpectrum(MCFileHandler::TriggerClass::EJ2),
                                      binningdet)},        
            {"EJ1", TriggerEfficiency(mcset.getDetLevelSpectrum(MCFileHandler::TriggerClass::INT7), 
                                      mcset.getDetLevelSpectrum(MCFileHandler::TriggerClass::EJ1),
                                      binningdet)},        
        };

        std::unordered_map<std::string, TH1 *> triggerEffsRebinned;
        for(auto &[trg, eff]: triggerEffs) {
            std::cout << "Building trigger efficiency for trigger " << trg << std::endl;
            auto efffine = eff.getEfficiencyFine();
            efffine->SetNameTitle(Form("TriggerEfficiency_%s_R%02d_orig", trg.data(), R), Form("Trigger efficiency for %s for R=%.1f (original)", trg.data(), radius));
            outhandler.setTriggerEff(R, efffine, trg, false);
            auto effrebinned = eff.getEfficiencyRebinned();
            effrebinned->SetNameTitle(Form("TriggerEfficiency_%s_R%02d_rebinned", trg.data(), R), Form("Trigger efficiency for %s for R=%.1f (rebinned)", trg.data(), radius));
            outhandler.setTriggerEff(R, effrebinned, trg, true);
            triggerEffsRebinned[trg] = effrebinned;
        }

        // Build the combined raw spectrum
        // Store histograms for different correction steps
        std::cout << "Running correction and normalization" << std::endl;
        std::map<std::string, TH1 *> mRawSpectraTriggerCorrected;
        for(const auto &trg : TRIGGERS) {
            auto spec = rawspectrumTrigger[trg];
            spec->SetNameTitle(Form("RawJetSpectrum_FullJets_R%02d_%s", R, trg.data()), Form("Raw spectrum full jets for R=%f for trigger %s", radius, trg.data()));
            auto rebinned = makeRebinnedSafe(spec, Form("RawJetSpectrum_FullJets_R%02d_%s_rebinned", R, trg.data()), binningdet);
            rebinned->SetTitle(Form("Rebinned raw spectrum full jets for R=%f for trigger %s", radius, trg.data()));
            auto normalized = histcopy(rebinned);
            normalized->SetNameTitle(Form("RawJetSpectrum_FullJets_R%02d_%s_normalized", R, trg.data()), Form("Normalized raw spectrum full jets for R=%f for trigger %s", radius, trg.data()));
            normalized->Scale(1./norms[trg]);
            auto corrected = histcopy(normalized);
            corrected->SetNameTitle(Form("RawJetSpectrum_FullJets_R%02d_%s_corrected", R, trg.data()), Form("Corrected raw spectrum full jets for R=%f for trigger %s", radius, trg.data()));
            if(trg.find("INT7") == std::string::npos) {
                // Correct for the trigger efficiency (in case the trigger is not min. bias)
                corrected->Divide(triggerEffsRebinned[trg]);
            } 
            mRawSpectraTriggerCorrected[trg] = corrected;
            outhandler.setRawHistTrigger(R, spec, trg, false);
            outhandler.setRawHistTrigger(R, rebinned, trg, true);
            outhandler.setNormalizedRawSpectrumTrigger(R, normalized, trg);
            outhandler.setTrgEffCorrectedRawSpectrumTrigger(R, corrected, trg);
        }
        TH1 *hrawOrig = makeCombinedRawSpectrum(*mRawSpectraTriggerCorrected["INT7"], *mRawSpectraTriggerCorrected["EJ2"], 50., *mRawSpectraTriggerCorrected["EJ2"], 100.);
        hrawOrig->SetNameTitle(Form("hrawOrig_R%02d", R), Form("Combined raw Level spectrum R=%.1f, before purity correction", radius));
        std::cout << "Having combined spectrum" << std::endl;

        auto jetFindingEff = mcset.getJetFindingEfficiency().makeEfficiency(binningpart),
             jetFindingEffClosure =closureset.getJetFindingEfficiency().makeEfficiency(binningpart),
             jetFindingPurity = mcset.getJetFindingPurity().makePurity(binningdet),
             jetFindingPurityClosure = closureset.getJetFindingPurity().makePurity(binningdet);
        jetFindingEff->SetNameTitle(Form("hJetfindingEfficiency_R%02d", R), Form("Jet finding efficiency for R = %.1f", radius));
        jetFindingEffClosure->SetNameTitle(Form("hJetfindingEfficiencyClosure_R%02d", R), Form("Jet finding efficiency for R = %.1f", radius));
        jetFindingPurity->SetNameTitle(Form("hJetfindingPurity_R%02d", R), Form("Jet finding purity for R = %.1f (closure test)", radius));
        jetFindingPurityClosure->SetNameTitle(Form("hJetfindingPurityClosure_R%02d", R), Form("Jet finding purity for R = %.1f (closure test)", radius));
        outhandler.setJetFindingEfficiency(R, jetFindingEff, false);
        outhandler.setJetFindingEfficiency(R, jetFindingEffClosure, true);
        outhandler.setJetFindingPurity(R, jetFindingPurity, false);
        outhandler.setJetFindingPurity(R, jetFindingPurityClosure, true);

        // Correct for the jet finding purity
        auto hraw = histcopy(hrawOrig);
        hraw->SetNameTitle(Form("hraw_R%02d", R), Form("Combined raw Level spectrum R=%.1f", radius));
        hraw->Multiply(jetFindingPurity);
        
        outhandler.setCombinedRawSpectrum(R, hrawOrig, false);
        outhandler.setCombinedRawSpectrum(R, hraw, true);

        // Get the MC objects
        std::cout << "Reading response matrix" << std::endl;
        auto ntrials = mcset.getTrials().getAverageTrials();
        outhandler.setMCScale(R, ntrials);
        auto &responsedata = mcset.getResponseMatrix();
        responsedata.Scale(ntrials);
        auto responsefine = responsedata.getHistogram();
        responsefine->SetName(Form("Rawresponse_R%02d_fine", R));
        ResponseHandler responsebuilder(responsedata, binningpart, binningdet);
        TH2 *responserebinned =  responsebuilder.getRebinnnedResponse();
        responserebinned->SetName(Form("Rawresponse_R%02d_standard", R));
        auto effkine = responsebuilder.makeKinematicEfficiency(),
             truefull = responsebuilder.makeFullyEfficienctTruthSpectrum();
        effkine->SetName(Form("effKine_R%02d", R));
        truefull->SetName(Form("truefull_R%02d", R));
        auto partlevel = mchandler.getMCSet(R).getPartLevelSpectrum().getHistogram();
        partlevel = partlevel->Rebin(binningpart.size()-1, Form( "hTruthFullyEfficient_R%02d", R), binningpart.data()); 

        // Get the MC closure objects
        auto &responsedataclosure = closureset.getResponseMatrix();
        responsedataclosure.Scale(ntrials);
        auto responseclosurefine = responsedataclosure.getHistogram();
        responseclosurefine->SetName(Form("Rawresponse_R%02d_fine", R));
        ResponseHandler closureresponsebuilder(responsedataclosure, binningpart, binningdet);
        TH2 *responseclosurerebinned =  closureresponsebuilder.getRebinnnedResponse();
        responseclosurerebinned->SetName(Form("Rawresponse_R%02d_standard", R));
        auto priorsclosure = closureresponsebuilder.makeFullyEfficienctTruthSpectrum();
        priorsclosure->SetName(Form("priorsclosure_R%02d", R));

        // det level truth must be taken from the purity histogram of the no-response sample
        // since the response matrix itself is restricted to matched jets
        std::cout << "Reading auxiliary files for closure test" << std::endl;
        auto detclosureNoSub = closureset.getTruthJetFindingPurity().makeProjectionAll(binningdet);
        detclosureNoSub->SetNameTitle(Form("detclosure_R%02d_NoCorr", R), Form("Det. level spectrum for closure test before correction, R=%.1f", radius));
        auto detclosure = histcopy(detclosureNoSub);
        detclosure->SetNameTitle(Form("detclosure_R%02d", R), Form("Det. level spectrum for closure test before correction, R=%.1f", radius));
        detclosure->Multiply(jetFindingPurityClosure);
        ResponseHandler closuretruthhandler(closureset.getTruthMatrix(), binningpart, binningdet);
        auto truthclosure = closuretruthhandler.makeFullyEfficienctTruthSpectrum();
        truthclosure->SetNameTitle(Form("partclosure_R%02d", R), Form("Part. level spectrum matched jets non-response sample, R=%.1f", radius));
        auto closurefulleff = mchandler.getClosureSet(R).getPartLevelTrue().getHistogram();
        closurefulleff = closurefulleff->Rebin(binningpart.size()-1, Form( "hTruthFullyEfficient_R%02d_Closure", R), binningpart.data()); 
        closurefulleff->SetTitle(Form("Full part. level spectrum non-response sample, R=%.1f", radius));

        // Register MC object for writing
        outhandler.setResponseMatrix(R, responsefine, true);
        outhandler.setResponseMatrix(R, responserebinned, false);
        outhandler.setResponseMatrixClosure(R, responseclosurefine, true);
        outhandler.setResponseMatrixClosure(R, responseclosurerebinned, false);
        outhandler.setMCTruth(R, truefull, false);
        outhandler.setMCTruth(R, partlevel, true);
        outhandler.setKinematicEfficiency(R, effkine);
        outhandler.setPriorsClosure(R, priorsclosure);
        outhandler.setDetLevelClosure(R, detclosureNoSub, false);
        outhandler.setDetLevelClosure(R, detclosure, true);
        outhandler.setPartLevelClosure(R, truthclosure, false);
        outhandler.setPartLevelClosure(R, closurefulleff, true);

        RooUnfoldResponse responsematrix(nullptr, truefull, responserebinned),
                          responsematrixClosure(nullptr, priorsclosure, responseclosurerebinned);
        responsematrix.UseOverflow(false);
        responsematrixClosure.UseOverflow(false);

        // Perform final unfolding
        
        UnfoldingPool work;
        for(auto ireg : ROOT::TSeqI(1, hraw->GetXaxis()->GetNbins())) {
            work.InsertWork({ireg, radius, hraw, jetFindingEff, &responsematrix, detclosure, &responsematrixClosure, jetFindingEffClosure});
        }

        std::set<UnfoldingResults> unfolding_results;
        if(doMT) {
            std::vector<std::thread> workthreads;
            std::mutex combinemutex;
            for(auto i : ROOT::TSeqI(0, 8)){
                workthreads.push_back(std::thread([&combinemutex, &work, &unfolding_results, unfoldingmethod](){
                    UnfoldingRunner worker(&work);
                    worker.getHandler().setUnfoldingMethod(unfoldingmethod);
                    worker.DoWork();
                    std::unique_lock<std::mutex> combinelock(combinemutex);
                    for(auto res : worker.getUnfolded()) unfolding_results.insert(res);
                }));
            }
            for(auto &th : workthreads) th.join();
        } else {
            UnfoldingRunner worker(&work);
            worker.getHandler().setUnfoldingMethod(unfoldingmethod);
            worker.DoWork();
            for(auto res : worker.getUnfolded()) unfolding_results.insert(res);
        };

        // Set output object
        for(auto reg : unfolding_results){
            outhandler.setUnfoldingResults(R, reg.fReg, reg.fUnfolded, reg.fNormalized, reg.fFullyCorrected, reg.fBackfolded, reg.fPearson, reg.fDvector,
                                              reg.fUnfoldedClosure, reg.fFullyCorrectedClosure, reg.fPearsonClosure, reg.fDvectorClosure);
        }
    }
    
    std::stringstream outfilename;
    outfilename << "correctedSVD_poor";
    if(sysvar.length()) {
        outfilename << "_" << sysvar;
    }
    if(radiusSel > 0) {
        outfilename << "_R" << std::setfill('0') << std::setw(2) << radiusSel;
    }
    outfilename << ".root";
    outhandler.write(outfilename.str());
}