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

void runNewCorrectionChainChargedEMCacc1DSVD_SpectrumTaskSimplePoor_CorrectEffPure(const std::string_view file2017 = "", const std::string_view file2018 = "", const std::string_view filemc = "", const std::string_view sysvar = "", int radiusSel = -1, bool doMT = false) {
    ROOT::EnableThreadSafety();
    const std::string jettype = "ChargedJets";
    
    UnfoldingHandler::UnfoldingMethod_t unfoldingmethod = UnfoldingHandler::UnfoldingMethod_t::kSVD;
    std::map<int, std::shared_ptr<DataFileHandler>> mDataFileHandlers;
    if(file2017.length() && file2017 != "None") {
        std::cout << "Reading file for 2017: " << file2017 << std::endl;
        auto filehandler = std::make_shared<DataFileHandler>(file2017, jettype, sysvar);
        mDataFileHandlers[2017] = filehandler;
    }
    if(file2018.length() && file2018 != "None") {
        std::cout << "Reading file for 2018: " << file2018 << std::endl;
        auto filehandler = std::make_shared<DataFileHandler>(file2018, jettype, sysvar);
        mDataFileHandlers[2018] = filehandler;
    }
    MCFileHandler mchandler(filemc, jettype,  sysvar);
    OutputHandler outhandler;

    std::vector<double> binningdet = getJetPtBinningNonLinSmearCharged(),
                        binningpart = getJetPtBinningNonLinTrueCharged();
    for(auto R : ROOT::TSeqI(2, 7)) {
        double radius = double(R) / 10.;
        if(radiusSel > 0 && R != radiusSel) {
            continue;
        }
        std::cout << "Doing jet radius " << radius << std::endl;

        // Adding spectra for each trigger class from the different years
        // add also integrated luminosity, scaled by the corresponding vertex finding efficiencies
        TH1 * rawspectrumCombined = nullptr;
        double norm = 0.;
        std::map<int, double> crossections = {{2017, 58.10}, {2018, 57.52}};
        for(auto &[year, filehandler] : mDataFileHandlers) {
            auto effVtx = filehandler->getVertexFindingEfficiency(R, "INT7").evaluate();
            outhandler.setVertexFindingEfficiency(R, year, effVtx);
                
            auto spec =  filehandler->getSpectrumAbs(R, "INT7").getSpectrumForTriggerCluster(TriggerCluster::kANY);
            if(!rawspectrumCombined) {
                rawspectrumCombined = spec;
            } else {
                rawspectrumCombined->Add(spec);
            }
            // luminosity from Luminosity handler does not include vertex finding efficiency, which needs to be evaluated by the user
            norm += filehandler->getClusterCounterAbs(R, "INT7").getCounters(TriggerCluster::kANY) / (effVtx * crossections.find(year)->second);
            outhandler.setRawEvents(R, year, "INT7", filehandler->getClusterCounterAbs(R, "INT7").getCounters(TriggerCluster::kANY));
        }

        auto &mcset = mchandler.getMCSet(R);
        auto &closureset = mchandler.getClosureSet(R);

        // Build the combined raw spectrum
        // Store histograms for different correction steps
        std::cout << "Running rebinning and normalization" << std::endl;
        rawspectrumCombined->SetNameTitle(Form("RawJetSpectrum_ChargedJets_R%02d_INT7", R), Form("Raw spectrum charged jets for R=%f for trigger INT7", radius));
            
        auto rebinned = makeRebinnedSafe(rawspectrumCombined, Form("RawJetSpectrum_ChargedJets_R%02d_INT7_rebinned", R), binningdet);
        rebinned->SetTitle(Form("Rebinned raw spectrum charged jets for R=%f for trigger INT7", radius));
        auto normalized = histcopy(rebinned);
        normalized->SetNameTitle(Form("RawJetSpectrum_ChargedJets_R%02d_INT7_normalized", R), Form("Normalized raw spectrum charged jets for R=%f for trigger INT7", radius));
        normalized->Scale(1./norm);
        outhandler.setRawHistTrigger(R, rawspectrumCombined, "INT7", false);
        outhandler.setNormalizedRawSpectrumTrigger(R, normalized, "INT7");
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
        auto hraw = histcopy(normalized);
        hraw->SetNameTitle(Form("hraw_R%02d", R), Form("Combined raw Level spectrum R=%.1f", radius));
        hraw->Multiply(jetFindingPurity);
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
                    worker.getHandler().setAcceptanceType(UnfoldingHandler::AcceptanceType_t::kEMCALFID);
                    worker.DoWork();
                    std::unique_lock<std::mutex> combinelock(combinemutex);
                    for(auto res : worker.getUnfolded()) unfolding_results.insert(res);
                }));
            }
            for(auto &th : workthreads) th.join();
        } else {
            UnfoldingRunner worker(&work);
            worker.getHandler().setUnfoldingMethod(unfoldingmethod);
            worker.getHandler().setAcceptanceType(UnfoldingHandler::AcceptanceType_t::kEMCALFID);
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