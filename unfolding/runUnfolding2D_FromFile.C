#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/roounfold.C"
#include "../helpers/unfolding.C"

TH2 * makeCombinedTriggers(const std::map<std::string, TH2 *> rawtriggers, double ptminEJ2, double ptminEJ1, const char *observable) {
    auto result = static_cast<TH2 *>(rawtriggers.find("INT7")->second->Clone(Form("h%sVsPtRawCombined", observable)));
    result->SetDirectory(nullptr);
    auto histEJ2 = rawtriggers.find("EJ2")->second;
    for(auto yb : ROOT::TSeqI(result->GetYaxis()->FindBin(ptminEJ2), result->GetYaxis()->FindBin(ptminEJ1))) {
        printf("Replacing pt-bin %d (%.1f GeV/c - %.1f GeV/c) - EJ2 (%s)\n", yb, result->GetYaxis()->GetBinLowEdge(yb), result->GetYaxis()->GetBinUpEdge(yb), histEJ2->GetName());
        for(auto xb : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(xb+1, yb, histEJ2->GetBinContent(xb+1, yb));
            result->SetBinError(xb+1, yb, histEJ2->GetBinError(xb+1, yb));
        }
    }
    auto histEJ1 = rawtriggers.find("EJ1")->second;
    for(auto yb : ROOT::TSeqI(result->GetYaxis()->FindBin(ptminEJ1), result->GetYaxis()->GetNbins()+1)) {
        printf("Replacing pt-bin %d (%.1f GeV/c - %.1f GeV/c) - EJ1 (%s)\n", yb, result->GetYaxis()->GetBinLowEdge(yb), result->GetYaxis()->GetBinUpEdge(yb), histEJ1->GetName());
        for(auto xb : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(xb+1, yb, histEJ1->GetBinContent(xb+1, yb));
            result->SetBinError(xb+1, yb, histEJ1->GetBinError(xb+1, yb));
        }
    }
    return result;
}

void runUnfolding2D_FromFile(const char *filedata, const char *fileresponse, const char *observablename = "all", const char *jetrstring = "all"){
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
    const double kMinPtEJ2 = 60., kMinPtEJ1 = 100.;
    for(auto observable : observablesSelected) outputwriter->mkdir(observable.data());

    RooUnfold::ErrorTreatment errtreatment = RooUnfold::kCovToy;

    auto create_histfinder = [](int iter) {
        return [iter] (const TH2 *hist) { if(std::string_view(hist->GetName()).find(Form("Iter%d", iter)) != std::string::npos) return true; else return false; };
    };
    
    for(auto R : rvalues){
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        std::cout << "Processing " << rtitle << std::endl;
        datareader->cd(rstring.data());
        auto datadirectory = gDirectory;
        responsereader->cd(Form("Response_%s", rstring.data()));
        auto responsedirectory = gDirectory;
        for(auto observable : observablesSelected){
            std::cout << "Unfolding observable " << observable << " ... " << std::endl;
            std::map<std::string, TH2 *> rawtriggers;
            for(const auto &t : triggers) {
                auto hist = static_cast<TH2 *>(datadirectory->Get(Form("h%sVsPt%sCorrected", observable.data(), t.data())));
                hist->SetDirectory(nullptr);
                rawtriggers[t] = hist;
            }
            auto rawcombined = makeCombinedTriggers(rawtriggers, kMinPtEJ2, kMinPtEJ1, observable.data());

            responsedirectory->cd("response");
            auto responsematrix = static_cast<RooUnfoldResponse *>(gDirectory->Get(Form("Responsematrix%s_%s", observable.data(), rstring.data())));
            auto effKine = static_cast<TH2 *>(gDirectory->Get(Form("EffKine%s_%s", observable.data(), rstring.data())));
            effKine->SetDirectory(nullptr);

            // get stuff for closure test
            responsedirectory->cd("closuretest");
            auto responsematrixClosure = static_cast<RooUnfoldResponse *>(gDirectory->Get(Form("ResponsematrixClosure%s_%s", observable.data(), rstring.data())));
            auto effKineClosure = static_cast<TH2 *>(gDirectory->Get(Form("EffKineClosure%s_%s", observable.data(), rstring.data())));
            effKine->SetDirectory(nullptr);
            auto detLevelClosure = static_cast<TH2 *>(gDirectory->Get(Form("closuredet%s_%s", observable.data(), rstring.data()))),
                 partLevelClosure = static_cast<TH2 *>(gDirectory->Get(Form("closuretruth%s_%s", observable.data(), rstring.data())));
            detLevelClosure->SetDirectory(nullptr);
            partLevelClosure->SetDirectory(nullptr);

            std::vector<TH2 *> unfoldedHists, refoldedHists, correctedHists,
                               unfoldedHistsClosure, refoldedHistsClosure, correctedHistsClosure;

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
                correctedHists.push_back(corrected);

                // MC closure test
                RooUnfoldBayes unfolderClosure(responsematrixClosure, detLevelClosure, iter);
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
                correctedHistsClosure.push_back(correctedClosure);
            }

            // Write to file
            outputwriter->cd(observable.data());
            auto obsdir = gDirectory;
            obsdir->mkdir(rstring.data());
            obsdir->cd(rstring.data());
            auto routbase = gDirectory;

            routbase->mkdir("rawlevel");
            routbase->cd("rawlevel");
            for(auto t : triggers) {
                auto triggerhist = rawtriggers[t];
                triggerhist->SetNameTitle(Form("rawlevel%s_%s_%s", observable.data(), rstring.data(), t.data()), Form("Raw %s distribution for %s, trigger %s", observable.data(), rtitle.data(), t.data()));
                triggerhist->Write();
            }
            rawcombined->SetNameTitle(Form("rawlevel%s_%s_combined", observable.data(), rstring.data()), Form("Raw %s distribution for %s, all triggers", observable.data(), rtitle.data()));
            rawcombined->Write();

            routbase->mkdir("response");
            routbase->cd("response");
            responsematrix->Write();
            effKine->Write();

            routbase->mkdir("closuretest");
            routbase->cd("closuretest");
            responsematrixClosure->Write();
            effKineClosure->Write();
            detLevelClosure->Write();
            partLevelClosure->Write();

            for(auto iter : ROOT::TSeqI(1, 31)) {
                std::string iterdir = Form("Iter%d", iter);
                routbase->mkdir(iterdir.data());
                routbase->cd(iterdir.data());
                auto histfinder = create_histfinder(iter);
                (*std::find_if(unfoldedHists.begin(), unfoldedHists.end(), histfinder))->Write();
                (*std::find_if(refoldedHists.begin(), refoldedHists.end(), histfinder))->Write();
                (*std::find_if(correctedHists.begin(), correctedHists.end(), histfinder))->Write();
                (*std::find_if(unfoldedHistsClosure.begin(), unfoldedHistsClosure.end(), histfinder))->Write();
                (*std::find_if(refoldedHistsClosure.begin(), refoldedHistsClosure.end(), histfinder))->Write();
                (*std::find_if(correctedHistsClosure.begin(), correctedHistsClosure.end(), histfinder))->Write();
            }
        }
    }
}