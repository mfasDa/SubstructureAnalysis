#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/roounfold.C"
#include "../helpers/unfolding.C"

TH2 * makeCombinedTriggers(const std::map<std::string, TH2 *> rawtriggers, double ptminEJ2, double ptminEJ1, const char *observable) {
    auto result = static_cast<TH2 *>(rawtriggers.find("INT7")->second->Clone(Form("h%sVsPtRaw", observable)));
    result->SetDirectory(nullptr);
    auto histEJ2 = rawtriggers.find("EJ2")->second;
    for(auto yb : ROOT::TSeqI(result->GetYaxis()->FindBin(ptminEJ2), result->GetYaxis()->FindBin(ptminEJ1))) {
        printf("Replacing pt-bin %d (%.1f GeV/c - %.1f GeV/c) - EJ2\n", yb, result->GetYaxis()->GetBinLowEdge(yb), result->GetYaxis()->GetBinUpEdge(yb));
        for(auto xb : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(xb+1, yb, histEJ2->GetBinContent(xb+1, yb));
            result->SetBinError(xb+1, yb, histEJ2->GetBinError(xb+1, yb));
        }
    }
    auto histEJ1 = rawtriggers.find("EJ2")->second;
    for(auto yb : ROOT::TSeqI(result->GetYaxis()->FindBin(ptminEJ1), result->GetYaxis()->GetNbins()+1)) {
        printf("Replacing pt-bin %d (%.1f GeV/c - %.1f GeV/c) - EJ1\n", yb, result->GetYaxis()->GetBinLowEdge(yb), result->GetYaxis()->GetBinUpEdge(yb));
        for(auto xb : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(xb+1, yb, histEJ1->GetBinContent(xb+1, yb));
            result->SetBinError(xb+1, yb, histEJ1->GetBinError(xb+1, yb));
        }
    }
    return result;
}

void runUnfolding2D_FromFile(const char *filedata, const char *fileresponse, const char *observable){
    std::unique_ptr<TFile> datareader(TFile::Open(filedata, "READ")),
                           responsereader(TFile::Open(fileresponse, "READ")),
                           outputwriter(TFile::Open(Form("Unfolded_%s.root", observable), "RECREATE"));
    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};
    const double kMinPtEJ2 = 60., kMinPtEJ1 = 100.;

    RooUnfold::ErrorTreatment errtreatment = RooUnfold::kCovToy;

    auto create_histfinder = [](int iter) {
        return [iter] (const TH2 *hist) { if(std::string_view(hist->GetName()).find(Form("Iter%d", iter)) != std::string::npos) return true; else return false; };
    };
    
    for(auto R : ROOT::TSeqI(2, 7)){
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        datareader->cd(rstring.data());
        std::map<std::string, TH2 *> rawtriggers;
        for(const auto &t : triggers) {
            auto hist = static_cast<TH2 *>(gDirectory->Get(Form("h%sVsPt%sCorrected", observable, t.data())));
            hist->SetDirectory(nullptr);
            rawtriggers[t] = hist;
        }
        auto rawcombined = makeCombinedTriggers(rawtriggers, kMinPtEJ2, kMinPtEJ1, observable);

        responsereader->cd(Form("Response_%s", rstring.data()));
        auto responsematrix = static_cast<RooUnfoldResponse *>(gDirectory->Get(Form("Responsematrix%s_%s", observable, rstring.data())));
        auto effKine = static_cast<TH2 *>(gDirectory->Get(Form("EffKine%s_%s", observable, rstring.data())));
        effKine->SetDirectory(nullptr);
        
        std::vector<TH2 *> unfoldedHists, refoldedHists, correctedHists;

        for(auto iter : ROOT::TSeqI(1, 31)) {
            std::cout << "Iteration " << iter << std::endl;
            RooUnfoldBayes unfolder(responsematrix, rawcombined, iter);
            auto unfolded = static_cast<TH2 *>(unfolder.Hreco(errtreatment));
            unfolded->SetNameTitle(Form("unfoldedIter%d_%s_%s", iter, observable, rstring.data()), Form("Unfolded distribution for %s for %s (iteration %d)", observable, rtitle.data(), iter));
            unfoldedHists.push_back(unfolded);

            // back-folding test
            auto refolded = Refold(rawcombined, unfolded, *responsematrix);
            refolded->SetDirectory(nullptr);
            refolded->SetNameTitle(Form("refoldedIter%d_%s_%s", iter, observable, rstring.data()), Form("Re-folded distribution for %s for %s (iteration %d)", observable, rtitle.data(), iter));
            refoldedHists.push_back(refolded);

            // efficiency correction
            auto corrected = static_cast<TH2 *>(unfolded->Clone());
            corrected->SetDirectory(nullptr);
            corrected->SetNameTitle(Form("correctedIter%d_%s_%s", iter, observable, rstring.data()), Form("Corrected distribution for %s for %s (iteration %d)", observable, rtitle.data(), iter));
            corrected->Divide(effKine);
            correctedHists.push_back(corrected);
        }

        // Write to file
        outputwriter->mkdir(rstring.data());
        outputwriter->cd(rstring.data());
        auto routbase = gDirectory;

        routbase->mkdir("rawlevel");
        routbase->cd("rawlevel");
        for(auto t : triggers) {
            auto triggerhist = rawtriggers[t];
            triggerhist->SetNameTitle(Form("rawlevel%s_%s_%s", observable, rstring.data(), t.data()), Form("Raw %s distribution for %s, trigger %s", observable, rtitle.data(), t.data()));
            triggerhist->Write();
        }
        rawcombined->SetNameTitle(Form("rawlevel%s_%s_combined", observable, rstring.data()), Form("Raw %s distribution for %s, all triggers", observable, rtitle.data()));
        rawcombined->Write();

        routbase->mkdir("response");
        responsematrix->Write();
        effKine->Write();

        for(auto iter : ROOT::TSeqI(1, 31)) {
            std::string iterdir = Form("Iter%d", iter);
            routbase->mkdir(iterdir.data());
            routbase->cd(iterdir.data());
            auto histfinder = create_histfinder(iter);
            (*std::find_if(unfoldedHists.begin(), unfoldedHists.end(), histfinder))->Write();
            (*std::find_if(refoldedHists.begin(), refoldedHists.end(), histfinder))->Write();
            (*std::find_if(correctedHists.begin(), correctedHists.end(), histfinder))->Write();
        }
    }
}