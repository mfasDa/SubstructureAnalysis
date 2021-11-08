#include "../../meta/stl.C"
#include "../../meta/root.C"

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

void makeCombinedRawDist(const char *inputfile = "rawsoftdrop.root"){
    std::unique_ptr<TFile> datareader(TFile::Open(inputfile, "READ")),
                           outputwriter(TFile::Open("combinedsoftdrop.root", "RECREATE"));
    
    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"},
                             observables = {"Zg", "Rg", "Nsd", "Thetag"};

    auto create_histfinder = [](int iter) {
        return [iter] (const TH2 *hist) { if(std::string_view(hist->GetName()).find(Form("Iter%d", iter)) != std::string::npos) return true; else return false; };
    };

    const double kMinPtEJ2 = 60., kMinPtEJ1 = 100.;
    std::vector<int> rvalues;
    for(auto R : ROOT::TSeqI(2, 7)) rvalues.push_back(R);
    
    for(auto R : rvalues){
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        std::cout << "Processing " << rtitle << std::endl;
        datareader->cd(rstring.data());
        auto datadirectory = static_cast<TDirectory *>(gDirectory);
        outputwriter->mkdir(rstring.data());
        for(auto observable : observables){
            std::cout << "Combining observable " << observable << " ... " << std::endl;
            std::map<std::string, TH2 *> rawtriggers;
            for(const auto &t : triggers) {
                auto hist = static_cast<TH2 *>(datadirectory->Get(Form("h%sVsPt%sCorrected", observable.data(), t.data())));
                hist->SetDirectory(nullptr);
                rawtriggers[t] = hist;
            }
            auto rawcombined = makeCombinedTriggers(rawtriggers, kMinPtEJ2, kMinPtEJ1, observable.data());

            // Write all to file
            outputwriter->cd(rstring.data());
            rawcombined->Write();
            for(auto [trg, hist] : rawtriggers) hist->Write();
        }
    }
}