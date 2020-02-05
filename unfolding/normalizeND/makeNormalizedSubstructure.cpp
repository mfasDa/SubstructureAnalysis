#include "../../meta/stl.C"
#include "../../meta/root.C"

struct TriggerEfficiencyContainer {
    int radius;
    std::map<std::string, TH1 *> triggerefficiencies;
};

std::vector<TriggerEfficiencyContainer> extractTriggerEfficiencies(const char *filemc, std::vector<double> ptbinning) {
    std::unique_ptr<TFile> reader(TFile::Open(filemc, "READ"));
    std::vector<TriggerEfficiencyContainer> result;
    std::vector<std::string> triggers = {"EJ1", "EJ2"};

    for(auto R : ROOT::TSeqI(2,7)){
        TriggerEfficiencyContainer cont;
        cont.radius = R;

        reader->cd(Form("JetSpectrum_FullJets_R%02d_INT7_tc200", R));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>(); 
        auto specmb2D = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
        std::unique_ptr<TH1> specMBFine(specmb2D->ProjectionY("INT7", 1, 1)),
                             specMBRebin(specMBFine->Rebin(ptbinning.size()-1, "INT7rebin", ptbinning.data()));
        for(auto trg : triggers) {
            reader->cd(Form("JetSpectrum_FullJets_R%02d_%s_tc200", R, trg.data()));
            histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>(); 
            auto spec2D = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
            std::unique_ptr<TH1> specTriggerFine(spec2D->ProjectionY(trg.data(), 1, 1));
            auto efficiency = specTriggerFine->Rebin(ptbinning.size()-1, Form("TriggerEfficiency_%s_R%02d", trg.data(), R), ptbinning.data());
            efficiency->SetDirectory(nullptr);
            efficiency->Divide(efficiency, specMBRebin.get(), 1., 1., "b");
            cont.triggerefficiencies[trg] = efficiency;
        }
        result.emplace_back(cont);
    }

    return result;
}

void makeNormalizedSubstructure(const char *filedata, const char *filemc) {
    const double kVerySmall = 1e-5;
    std::vector<double> ptbinning = {10., 15., 20., 25., 30., 35., 40., 50., 60., 80., 100., 120., 140., 160., 180., 200.};

    auto triggerefficiencies = extractTriggerEfficiencies(filemc, ptbinning);
    std::unique_ptr<TFile> reader(TFile::Open(filedata, "READ")),
                           writer(TFile::Open("rawsoftdrop.root", "RECREATE"));
    reader->ls();
    std::vector<std::string> triggers = {"INT7", "EJ2", "EJ1"},
                             observables = {"Zg", "Rg", "Thetag", "Nsd"};

    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string outdirname(Form("R%02d", R));
        writer->mkdir(outdirname.data());
        for(auto trg : triggers) {
            reader->cd(Form("SoftDropResponse_FullJets_R%02d_%s", R, trg.data()));
            auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
            histlist->ls();
            double weight = 1;
            if(trg == "EJ1") {
                // Scale for the additional cluster luminosity
                auto trgcounter = static_cast<TH1 *>(histlist->FindObject("fHistTriggerClasses"));
                double ntrgcent = trgcounter->GetBinContent(trgcounter->GetXaxis()->FindBin("CEMC7EJ1-B-NOPF-CENT")),
                       ntrgcentnotrd = trgcounter->GetBinContent(trgcounter->GetXaxis()->FindBin("CEMC7EJ1-B-NOPF-CENTNOTRD"));
                weight = ntrgcent/ntrgcentnotrd;
                std::cout << "Applying downscale weight " << weight << " for trigger " << trg << std::endl;
            }

            TH1 *triggereff = nullptr;
            if(trg != "INT7") {
                std::cout << "Getting trigger efficiency for trigger " << trg << std::endl;
                auto cont = std::find_if(triggerefficiencies.begin(), triggerefficiencies.end(), [R](const TriggerEfficiencyContainer &cont) { return cont.radius == R; });
                if(cont != triggerefficiencies.end()) {
                    auto effcurve = cont->triggerefficiencies.find(trg);
                    if(effcurve != cont->triggerefficiencies.end()) {
                        std::cout << "Found trigger efficiency for trigger " << trg << " and radius " << double(R)/10. << std::endl;
                        triggereff = effcurve->second;
                    }
                }
            }

            writer->cd(outdirname.data());
            if(triggereff) triggereff->Write();

            for(auto observable : observables) {
                auto rawhist = static_cast<TH2 *>(histlist->FindObject(Form("h%sVsPtWeighted", observable.data())));
                rawhist->SetDirectory(nullptr);
                rawhist->SetNameTitle(Form("h%sVsPt%sRaw", observable.data(), trg.data()), Form("%s vs. Pt for trigger %s (raw)", observable.data(), trg.data()));
                rawhist->Scale(weight);

                std::vector<double> obsbinning;
                obsbinning.emplace_back(rawhist->GetXaxis()->GetBinLowEdge(1));
                for(auto bin : ROOT::TSeq(0, rawhist->GetXaxis()->GetNbins())) obsbinning.emplace_back(rawhist->GetXaxis()->GetBinUpEdge(bin+1));

                auto resulthist = new TH2D(Form("h%sVsPt%sCorrected", observable.data(), trg.data()), Form("%s vs. Pt for trigger %s (corrected)", observable.data(), trg.data()), obsbinning.size()-1, obsbinning.data(), ptbinning.size()-1, ptbinning.data());
                resulthist->SetDirectory(nullptr);

                for(auto ipt : ROOT::TSeqI(0, resulthist->GetYaxis()->GetNbins())) {
                    auto effval = triggereff ? 1./triggereff->GetBinContent(ipt+1) : 1.;
                    std::unique_ptr<TH1> slice(rawhist->ProjectionX("slice", rawhist->GetYaxis()->FindBin(resulthist->GetYaxis()->GetBinLowEdge(ipt) + kVerySmall), rawhist->GetYaxis()->FindBin(resulthist->GetYaxis()->GetBinUpEdge(ipt) - kVerySmall)));
                    slice->Scale(effval);
                    for(auto iobs : ROOT::TSeqI(0, resulthist->GetXaxis()->GetNbins())) { 
                        resulthist->SetBinContent(iobs+1, ipt+1, slice->GetBinContent(iobs+1));
                    }
                }

                rawhist->Write();
                resulthist->Write();
            }
        }
    }
}