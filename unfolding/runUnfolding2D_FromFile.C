#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/roounfold.C"
#include "../helpers/unfolding.C"

TH2 * makeCombinedTriggers(const std::map<std::string, TH2 *> rawtriggers, double ptminEJ2, double ptminEJ1, const char *observable) {
    const double kVerySmall = 1e-5;
    auto result = static_cast<TH2 *>(rawtriggers.find("INT7")->second->Clone(Form("h%sVsPtRawCombined", observable)));
    result->SetDirectory(nullptr);
    auto histEJ2 = rawtriggers.find("EJ2")->second;
    for(auto yb : ROOT::TSeqI(result->GetYaxis()->FindBin(ptminEJ2 + kVerySmall), result->GetYaxis()->FindBin(ptminEJ1 + kVerySmall))) {
        printf("Replacing pt-bin %d (%.1f GeV/c - %.1f GeV/c) - EJ2 (%s)\n", yb, result->GetYaxis()->GetBinLowEdge(yb), result->GetYaxis()->GetBinUpEdge(yb), histEJ2->GetName());
        for(auto xb : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(xb+1, yb, histEJ2->GetBinContent(xb+1, yb));
            result->SetBinError(xb+1, yb, histEJ2->GetBinError(xb+1, yb));
        }
    }
    auto histEJ1 = rawtriggers.find("EJ1")->second;
    for(auto yb : ROOT::TSeqI(result->GetYaxis()->FindBin(ptminEJ1 + kVerySmall), result->GetYaxis()->GetNbins()+1)) {
        printf("Replacing pt-bin %d (%.1f GeV/c - %.1f GeV/c) - EJ1 (%s)\n", yb, result->GetYaxis()->GetBinLowEdge(yb), result->GetYaxis()->GetBinUpEdge(yb), histEJ1->GetName());
        for(auto xb : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(xb+1, yb, histEJ1->GetBinContent(xb+1, yb));
            result->SetBinError(xb+1, yb, histEJ1->GetBinError(xb+1, yb));
        }
    }
    return result;
}

void runUnfolding2D_FromFile(const char *filedata, const char *fileresponse, const char *observablename = "all", const char *binvar = "default", const char *jetrstring = "all", bool correctEffPure = true){
    std::stringstream outfilename;
    outfilename << "UnfoldedSD";
    if(std::string_view(observablename) != std::string_view("all")) outfilename << "_" << observablename;
    if(std::string_view(jetrstring) != std::string_view("all")) outfilename << "_" << jetrstring;
    outfilename << ".root";
    std::unique_ptr<TFile> datareader(TFile::Open(filedata, "READ")),
                           responsereader(TFile::Open(fileresponse, "READ")),
                           outputwriter(TFile::Open(outfilename.str().data(), "RECREATE"));
    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"},
                             observablesAll = {"Zg", "Rg", "Nsd", "Thetag"},
                             observablesSelected;
    if(std::string_view(observablename) == "all") observablesSelected = observablesAll;
    else observablesSelected.push_back(observablename);
    std::vector<int> rvalues;
    if(std::string_view(jetrstring) == "all"){
        for(auto R : ROOT::TSeqI(2, 7)) rvalues.push_back(R);
    } else {
        std::string rstr(jetrstring);
        auto rval = std::stoi(rstr.substr(1));
        rvalues.push_back(rval);
    }
    std::map<std::string, double> triggerSwapEJ2 = {
        {"default", 60.},
        {"truncationLowLoose", 60.},
        {"truncationLowStrong", 60.},
        {"truncationHighLoose", 60.},
        {"truncationHighStrong", 60.},
        {"binning1", 59.},
        {"binning2", 61.},
        {"binning3", 57.},
        {"binning4", 61.},
        {"ej2low", 55.},
        {"ej2high", 65.},
        {"ej1low", 60.},
        {"ej1high", 60.}
    };
    std::map<std::string, double> triggerSwapEJ1 = {
        {"default", 100.},
        {"truncationLowLoose", 100.},
        {"truncationLowStrong", 100.},
        {"truncationHighLoose", 100.},
        {"truncationHighStrong", 100.},
        {"binning1", 99.},
        {"binning2", 101.},
        {"binning3", 110.},
        {"binning4", 107.},
        {"ej2low", 95.},
        {"ej2high", 105.},
        {"ej1low", 100.},
        {"ej1high", 100.}
    };

    const double kMinPtEJ2 = 60., kMinPtEJ1 = 100.;
    for(auto observable : observablesSelected) outputwriter->mkdir(observable.data());

    RooUnfold::ErrorTreatment errtreatment = RooUnfold::kCovToy;

    auto create_objectfinder = [](int iter) {
        return [iter] (const TObject *hist) { if(std::string_view(hist->GetName()).find(Form("Iter%d", iter)) != std::string::npos) return true; else return false; };
    };

    for(auto R : rvalues){
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        std::cout << "Processing " << rtitle << std::endl;
        datareader->cd(rstring.data());
        auto datadirectory = static_cast<TDirectory *>(gDirectory);
        responsereader->cd(Form("Response_%s", rstring.data()));
        auto responsedirectory = static_cast<TDirectory *>(gDirectory);
        for(auto observable : observablesSelected){
            std::cout << "Unfolding observable " << observable << " ... " << std::endl;
            std::map<std::string, TH2 *> rawtriggers;
            for(const auto &t : triggers) {
                auto hist = static_cast<TH2 *>(datadirectory->Get(Form("h%sVsPt%sCorrected", observable.data(), t.data())));
                hist->SetDirectory(nullptr);
                rawtriggers[t] = hist;
            }
            auto rawcombined = makeCombinedTriggers(rawtriggers, triggerSwapEJ2[binvar], triggerSwapEJ1[binvar], observable.data()),
                 rawcombinedOriginal = static_cast<TH2 *>(rawcombined->Clone());
            rawcombinedOriginal->SetDirectory(nullptr);

            responsedirectory->cd("response");
            auto responsematrix = static_cast<RooUnfoldResponse *>(gDirectory->Get(Form("Responsematrix%s_%s", observable.data(), rstring.data())));
            auto effKine = static_cast<TH2 *>(gDirectory->Get(Form("EffKine%s_%s", observable.data(), rstring.data())));
            effKine->SetDirectory(nullptr);

            // get jet finding efficiency and purity (if available)
            TH2 *jetfindingeff(nullptr), *jetfindingpurity(nullptr),
                *jetfindingeffClosure(nullptr), *jetfindingpurityClosure(nullptr); 
            if(correctEffPure) {
                jetfindingeff = static_cast<TH2 *>(gDirectory->Get(Form("JetFindingEff_%s_R%02d", observable.data(), R))),
                jetfindingpurity = static_cast<TH2 *>(gDirectory->Get(Form("JetFindingPurity_%s_R%02d", observable.data(), R)));
                if(jetfindingeff) jetfindingeff->SetDirectory(nullptr);
                if(jetfindingpurity) jetfindingpurity->SetDirectory(nullptr);
                jetfindingeffClosure = static_cast<TH2 *>(gDirectory->Get(Form("JetFindingEffClosure_%s_R%02d", observable.data(), R))),
                jetfindingpurityClosure = static_cast<TH2 *>(gDirectory->Get(Form("JetFindingPurityClosure_%s_R%02d", observable.data(), R)));
                if(jetfindingeffClosure) jetfindingeffClosure->SetDirectory(nullptr);
                if(jetfindingpurityClosure) jetfindingpurityClosure->SetDirectory(nullptr);
            }

            // get stuff for closure test
            responsedirectory->cd("closuretest");
            auto responsematrixClosure = static_cast<RooUnfoldResponse *>(gDirectory->Get(Form("ResponsematrixClosure%s_%s", observable.data(), rstring.data())));
            auto effKineClosure = static_cast<TH2 *>(gDirectory->Get(Form("EffKineClosure%s_%s", observable.data(), rstring.data())));
            effKine->SetDirectory(nullptr);
            auto detLevelClosure = static_cast<TH2 *>(gDirectory->Get(Form("closuredet%s_%s", observable.data(), rstring.data()))),
                 partLevelClosure = static_cast<TH2 *>(gDirectory->Get(Form("closuretruth%s_%s", observable.data(), rstring.data()))),
                 partLevelClosureCut = static_cast<TH2 *>(gDirectory->Get(Form("closuretruthcut%s_%s", observable.data(), rstring.data())));
            detLevelClosure->SetDirectory(nullptr);
            partLevelClosure->SetDirectory(nullptr);
            if(partLevelClosureCut) partLevelClosureCut->SetDirectory(nullptr);
            TH2 *detLevelClosureAll(nullptr), *partLevelClosureAll(nullptr);
            if(correctEffPure) {
                detLevelClosureAll = static_cast<TH2 *>(gDirectory->Get(Form("closuredetAll%s_%s", observable.data(), rstring.data())));
                partLevelClosureAll = static_cast<TH2 *>(gDirectory->Get(Form("closuretruthAll%s_%s", observable.data(), rstring.data())));
                if(detLevelClosureAll) detLevelClosureAll->SetDirectory(nullptr);
                if(partLevelClosureAll) partLevelClosureAll->SetDirectory(nullptr);
            }

            // Select det. level for closure test
            // if purity correction requested use detLevelClosureAll
            // if purity correction not requested or detLevelClosureAll not available use detLevelClosure
            auto detLevelClosureUse = detLevelClosureAll ? detLevelClosureAll : detLevelClosure;
            auto detLevelClosureOriginal = static_cast<TH2 *>(detLevelClosureUse->Clone());
            detLevelClosureOriginal->SetDirectory(nullptr);

            std::vector<TH2 *> unfoldedHists, refoldedHists, correctedHists, correctedHistsNoEff,
                               unfoldedHistsClosure, refoldedHistsClosure, correctedHistsClosure, correctedHistsClosureNoEff;
            std::vector<TList *> pearsonPt, pearsonShape, pearsonClosurePt, pearsonClosureShape;

            if(jetfindingpurity) {
                // Correct rawspectrum for jet finding purity
                rawcombined->Multiply(jetfindingpurity);
            }
            if(jetfindingpurityClosure) {
                detLevelClosureUse->Multiply(jetfindingpurityClosure);
            }

            for(auto iter : ROOT::TSeqI(1, 31)) {
                std::cout << "Iteration " << iter << std::endl;
                RooUnfoldBayes unfolder(responsematrix, rawcombined, iter);
                auto unfolded = static_cast<TH2 *>(unfolder.Hreco(errtreatment));
                unfolded->SetNameTitle(Form("unfoldedIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Unfolded distribution for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                unfoldedHists.push_back(unfolded);
   
                // back-folding test
                auto refolded = Refold(rawcombined, unfolded, *responsematrix);
                refolded->SetDirectory(nullptr);
                refolded->SetNameTitle(Form("refoldedIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Re-folded distribution for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                refoldedHists.push_back(refolded);

                // efficiency correction
                auto corrected = static_cast<TH2 *>(unfolded->Clone());
                corrected->SetDirectory(nullptr);
                corrected->SetNameTitle(Form("correctedIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Corrected distribution for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                corrected->Divide(effKine);
                auto correctedNoJetFindingEff = static_cast<TH2 *>(corrected->Clone());
                correctedNoJetFindingEff->SetDirectory(nullptr);
                correctedNoJetFindingEff->SetNameTitle(Form("correctedNoJetFindingEffIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Corrected distribution except jet finding efficiency for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                if(jetfindingeff) corrected->Divide(jetfindingeff);
                correctedHists.push_back(corrected);
                correctedHistsNoEff.push_back(correctedNoJetFindingEff);
                
                // Pearson coefficients
                auto covmat = unfolder.Ereco((RooUnfold::ErrorTreatment)RooUnfold::kCovariance);
                auto histsPearsonShape = new TList,
                     histsPearsonPt = new TList;
                histsPearsonShape->SetName(Form("pearsonCoefficientsShape_Iter%d", iter));
                histsPearsonPt->SetName(Form("pearsonCoefficientsPt_Iter%d", iter));
                for(int ipt : ROOT::TSeqI(0, unfolded->GetYaxis()->GetNbins())) {
                    double pearsonptmin = unfolded->GetYaxis()->GetBinLowEdge(ipt+1),
                           pearsonptmax = unfolded->GetYaxis()->GetBinUpEdge(ipt+1);
                    auto pearsonHistShape = CorrelationHistShape(covmat, 
                                                                 Form("pearson%s_Iter%d_pt%d_%d", observable.data(), iter, int(pearsonptmin), int(pearsonptmax)), 
                                                                 Form("Pearson coefficients for %s (iteration %d) for %.1f GeV/c < p_{t,jet} < %.1f GeV/c", observable.data(), iter, pearsonptmin, pearsonptmax), 
                                                                 unfolded->GetXaxis()->GetNbins(), 
                                                                 unfolded->GetYaxis()->GetNbins(), 
                                                                 ipt);
                    pearsonHistShape->SetDirectory(nullptr);
                    histsPearsonShape->Add(pearsonHistShape);
                }
                for(auto ishape : ROOT::TSeqI(0, unfolded->GetXaxis()->GetNbins())){
                    double pearsonshapemin = unfolded->GetXaxis()->GetBinLowEdge(ishape+1),
                           pearsonshapemax = unfolded->GetXaxis()->GetBinUpEdge(ishape+1);
                    auto pearsonHistPt = CorrelationHistPt(covmat,
                                                           Form("pearsonPt_Iter%d_%s%d_%d", iter, observable.data(), int(pearsonshapemin * 100.), int(pearsonshapemax * 100.)), 
                                                           Form("Pearson coefficients for p_{t} (iteration %d) for %.1f < %s < %.1f", iter, pearsonshapemin, observable.data(), pearsonshapemax), 
                                                           unfolded->GetXaxis()->GetNbins(), 
                                                           unfolded->GetYaxis()->GetNbins(), 
                                                           ishape);
                    pearsonHistPt->SetDirectory(nullptr);
                    histsPearsonPt->Add(pearsonHistPt);
                }
                pearsonShape.push_back(histsPearsonShape);
                pearsonPt.push_back(histsPearsonPt);

                // MC closure test
                RooUnfoldBayes unfolderClosure(responsematrixClosure, detLevelClosureUse, iter);
                auto unfoldedClosure = static_cast<TH2 *>(unfolderClosure.Hreco(errtreatment));
                unfoldedClosure->SetNameTitle(Form("unfoldedClosureIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Unfolded distribution of the closure test for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                unfoldedHistsClosure.push_back(unfoldedClosure);
   
                // back-folding test
                auto refoldedClosure = Refold(detLevelClosure, unfoldedClosure, *responsematrixClosure);
                refoldedClosure->SetDirectory(nullptr);
                refoldedClosure->SetNameTitle(Form("refoldedClosureIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Re-folded distribution of the closure test for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                refoldedHistsClosure.push_back(refoldedClosure);

                // efficiency correction
                auto correctedClosure = static_cast<TH2 *>(unfoldedClosure->Clone());
                correctedClosure->SetDirectory(nullptr);
                correctedClosure->SetNameTitle(Form("correctedClosureIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Corrected distribution of the closure test for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                correctedClosure->Divide(effKineClosure);
                auto correctedClosureNoJetFindingEff = static_cast<TH2 *>(correctedClosure->Clone());
                correctedClosureNoJetFindingEff->SetDirectory(nullptr);
                correctedClosureNoJetFindingEff->SetNameTitle(Form("correctedClosureNoJetFindingEffIter%d_%s_%s", iter, observable.data(), rstring.data()), Form("Corrected distribution except jet finding efficiency of the closure test for %s for %s (iteration %d)", observable.data(), rtitle.data(), iter));
                if(jetfindingeffClosure) correctedClosure->Divide(jetfindingeffClosure);
                correctedHistsClosure.push_back(correctedClosure);
                correctedHistsClosureNoEff.push_back(correctedClosureNoJetFindingEff);

                // Pearson coefficients for the closure test
                auto covmatClosure = unfolderClosure.Ereco((RooUnfold::ErrorTreatment)RooUnfold::kCovariance);
                auto histsPearsonShapeClosure = new TList,
                     histsPearsonPtClosure = new TList;
                histsPearsonShapeClosure->SetName(Form("pearsonCoefficientsClosureShape_Iter%d", iter));
                histsPearsonPtClosure->SetName(Form("pearsonCoefficientsClosurePt_Iter%d", iter));
                for(int ipt : ROOT::TSeqI(0, unfoldedClosure->GetYaxis()->GetNbins())) {
                    double pearsonptmin = unfoldedClosure->GetYaxis()->GetBinLowEdge(ipt+1),
                           pearsonptmax = unfoldedClosure->GetYaxis()->GetBinUpEdge(ipt+1);
                    auto pearsonshape = CorrelationHistShape(covmat, 
                                                             Form("pearsonClosure%s_Iter%d_pt%d_%d", observable.data(), iter, int(pearsonptmin), int(pearsonptmax)), 
                                                             Form("Pearson coefficients for %s (iteration %d) for %.1f GeV/c < p_{t,jet} < %.1f GeV/c", observable.data(), iter, pearsonptmin, pearsonptmax), 
                                                             unfoldedClosure->GetXaxis()->GetNbins(), 
                                                             unfoldedClosure->GetYaxis()->GetNbins(), 
                                                             ipt);
                    pearsonshape->SetDirectory(nullptr);
                    histsPearsonShapeClosure->Add(pearsonshape);
                }
                for(auto ishape : ROOT::TSeqI(0, unfoldedClosure->GetXaxis()->GetNbins())){
                    double pearsonshapemin = unfolded->GetXaxis()->GetBinLowEdge(ishape+1),
                           pearsonshapemax = unfolded->GetXaxis()->GetBinUpEdge(ishape+1);
                    auto pearsonpt = CorrelationHistPt(covmat,
                                                       Form("pearsonClosurePt_Iter%d_%s%d_%d", iter, observable.data(), int(pearsonshapemin * 100.), int(pearsonshapemax * 100.)), 
                                                       Form("Pearson coefficients for p_{t} (iteration %d) for %.1f < %s < %.1f", iter, pearsonshapemin, observable.data(), pearsonshapemax), 
                                                       unfoldedClosure->GetXaxis()->GetNbins(), 
                                                       unfoldedClosure->GetYaxis()->GetNbins(), 
                                                       ishape);
                    pearsonpt->SetDirectory(nullptr);
                    histsPearsonPtClosure->Add(pearsonpt);
                }
                pearsonClosureShape.push_back(histsPearsonShapeClosure);
                pearsonClosurePt.push_back(histsPearsonPtClosure);
            }

            // Write to file
            outputwriter->cd(observable.data());
            auto obsdir = static_cast<TDirectory *>(gDirectory);
            obsdir->mkdir(rstring.data());
            obsdir->cd(rstring.data());
            auto routbase = static_cast<TDirectory *>(gDirectory);

            routbase->mkdir("rawlevel");
            routbase->cd("rawlevel");
            for(auto t : triggers) {
                auto triggerhist = rawtriggers[t];
                triggerhist->SetNameTitle(Form("rawlevel%s_%s_%s", observable.data(), rstring.data(), t.data()), Form("Raw %s distribution for %s, trigger %s", observable.data(), rtitle.data(), t.data()));
                triggerhist->Write();
            }
            rawcombined->SetNameTitle(Form("rawlevel%s_%s_combined", observable.data(), rstring.data()), Form("Raw %s distribution for %s, all triggers", observable.data(), rtitle.data()));
            rawcombined->Write();
            rawcombinedOriginal->SetNameTitle(Form("rawlevelOriginal%s_%s_combined", observable.data(), rstring.data()), Form("Raw %s distribution for %s, all triggers (Original)", observable.data(), rtitle.data()));
            rawcombinedOriginal->Write();
            if(jetfindingpurity) jetfindingpurity->Write();

            routbase->mkdir("response");
            routbase->cd("response");
            responsematrix->Write();
            effKine->Write();
            if(jetfindingeff) jetfindingeff->Write();

            routbase->mkdir("closuretest");
            routbase->cd("closuretest");
            responsematrixClosure->Write();
            effKineClosure->Write();
            detLevelClosure->Write();
            detLevelClosureOriginal->Write();
            partLevelClosure->Write();
            if(partLevelClosureCut) partLevelClosureCut->Write();
            if(detLevelClosureAll) detLevelClosureAll->Write();
            if(partLevelClosureAll) partLevelClosureAll->Write();

            for(auto iter : ROOT::TSeqI(1, 31)) {
                std::string iterdir = Form("Iter%d", iter);
                routbase->mkdir(iterdir.data());
                routbase->cd(iterdir.data());
                auto objectfinder = create_objectfinder(iter);
                (*std::find_if(unfoldedHists.begin(), unfoldedHists.end(), objectfinder))->Write();
                (*std::find_if(refoldedHists.begin(), refoldedHists.end(), objectfinder))->Write();
                (*std::find_if(correctedHists.begin(), correctedHists.end(), objectfinder))->Write();
                (*std::find_if(correctedHistsNoEff.begin(), correctedHistsNoEff.end(), objectfinder))->Write();
                (*std::find_if(pearsonShape.begin(), pearsonShape.end(), objectfinder))->Write();
                (*std::find_if(pearsonPt.begin(), pearsonPt.end(), objectfinder))->Write();
                (*std::find_if(unfoldedHistsClosure.begin(), unfoldedHistsClosure.end(), objectfinder))->Write();
                (*std::find_if(refoldedHistsClosure.begin(), refoldedHistsClosure.end(), objectfinder))->Write();
                (*std::find_if(correctedHistsClosure.begin(), correctedHistsClosure.end(), objectfinder))->Write();
                (*std::find_if(correctedHistsClosureNoEff.begin(), correctedHistsClosureNoEff.end(), objectfinder))->Write();
                (*std::find_if(pearsonClosureShape.begin(), pearsonClosureShape.end(), objectfinder))->Write();
                (*std::find_if(pearsonClosurePt.begin(), pearsonClosurePt.end(), objectfinder))->Write();
            }
        }
    }
}
