#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../binnings/binningPt2D.C"

struct TriggerEfficiencyContainer {
    int radius;
    std::map<std::string, TH1 *> triggerefficiencies;
    std::map<std::string, TH1 *> triggerefficienciesFine;
};

std::vector<TriggerEfficiencyContainer> extractTriggerEfficiencies(const char *filemc, std::vector<double> ptbinning) {
    std::unique_ptr<TFile> reader(TFile::Open(filemc, "READ"));
    std::vector<TriggerEfficiencyContainer> result;
    std::vector<std::string> triggers = {"EJ1", "EJ2"};
    std::vector<double> ptbinningrestricted;
    double ipt = 0.;
    while(ipt < 301.) {ptbinningrestricted.emplace_back(ipt); ipt += 1.; }

    for(auto R : ROOT::TSeqI(2,7)){
        TriggerEfficiencyContainer cont;
        cont.radius = R;

        reader->cd(Form("JetSpectrum_FullJets_R%02d_INT7_tc200", R));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>(); 
        auto specmb2D = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
        std::unique_ptr<TH1> specMBFine(specmb2D->ProjectionY("INT7Tmp", 1, 1)->Rebin(ptbinningrestricted.size()-1, "INT7", ptbinningrestricted.data())),
                             specMBRebin(specMBFine->Rebin(ptbinning.size()-1, "INT7rebin", ptbinning.data()));
        for(auto trg : triggers) {
            reader->cd(Form("JetSpectrum_FullJets_R%02d_%s_tc200", R, trg.data()));
            histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>(); 
            auto spec2D = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
            auto efficiencyFine = spec2D->ProjectionY(Form("TriggerEfficiencyFineTmp_%s_R%02d", trg.data(), R), 1, 1)->Rebin(ptbinningrestricted.size() - 1, Form("TriggerEfficiencyFine_%s_R%02d", trg.data(), R), ptbinningrestricted.data());
            efficiencyFine->SetDirectory(nullptr);
            auto efficiency = efficiencyFine->Rebin(ptbinning.size()-1, Form("TriggerEfficiency_%s_R%02d", trg.data(), R), ptbinning.data());
            efficiency->SetDirectory(nullptr);
            efficiencyFine->Divide(efficiencyFine, specMBFine.get(), 1., 1., "b");
            efficiency->Divide(efficiency, specMBRebin.get(), 1., 1., "b");
            cont.triggerefficienciesFine[trg] = efficiencyFine;
            cont.triggerefficiencies[trg] = efficiency;
        }
        result.emplace_back(cont);
    }

    return result;
}

TH2 *makeRawHistFromSparse(THnSparse *hsparse, int triggercluster){
    int first = hsparse->GetAxis(2)->GetFirst(),
        last = hsparse->GetAxis(2)->GetLast();
    hsparse->GetAxis(2)->SetRange(triggercluster+1, triggercluster+1);
    auto rawlevel = hsparse->Projection(1,0);
    rawlevel->SetDirectory(nullptr);
    hsparse->GetAxis(2)->SetRange(first,last);
    return rawlevel;
}

void makeNormalizedSubstructure(const char *filedata, const char *filemc, const char *binvarname = "default") {
    const double kVerySmall = 1e-5;
    std::vector<double> ptbinning = getDetPtBinning(binvarname);

    auto triggerefficiencies = extractTriggerEfficiencies(filemc, ptbinning);
    std::unique_ptr<TFile> reader(TFile::Open(filedata, "READ")),
                           writer(TFile::Open("rawsoftdrop.root", "RECREATE"));
    reader->ls();
    std::vector<std::string> triggers = {"INT7", "EJ2", "EJ1"},
                             observables = {"Zg", "Rg", "Thetag", "Nsd"};
    std::map<std::string, int> triggerclusters = {{"INT7", 0}, {"EJ2", 0}, {"EJ1", 1}};

    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string outdirname(Form("R%02d", R));
        writer->mkdir(outdirname.data());
        for(auto trg : triggers) {
            reader->cd(Form("SoftDropResponse_FullJets_R%02d_%s", R, trg.data()));
            auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
            double weight = 1;
            if(trg == "EJ1") {
                // Scale for the additional cluster luminosity
                auto trgcounter = static_cast<TH1 *>(histlist->FindObject("fHistTriggerClasses"));
                double ntrgcent = trgcounter->GetBinContent(trgcounter->GetXaxis()->FindBin("CEMC7EJ1-B-NOPF-CENT")),
                       ntrgcentnotrd = trgcounter->GetBinContent(trgcounter->GetXaxis()->FindBin("CEMC7EJ1-B-NOPF-CENTNOTRD"));
                weight = ntrgcent/ntrgcentnotrd;
                std::cout << "Applying downscale weight " << weight << " for trigger " << trg << std::endl;
            }

            TH1 *triggereff = nullptr,
                *triggereffFine = nullptr;
            if(trg != "INT7") {
                std::cout << "Getting trigger efficiency for trigger " << trg << std::endl;
                auto cont = std::find_if(triggerefficiencies.begin(), triggerefficiencies.end(), [R](const TriggerEfficiencyContainer &cont) { return cont.radius == R; });
                if(cont != triggerefficiencies.end()) {
                    auto effcurve = cont->triggerefficiencies.find(trg);
                    if(effcurve != cont->triggerefficiencies.end()) {
                        std::cout << "Found trigger efficiency for trigger " << trg << " and radius " << double(R)/10. << std::endl;
                        triggereff = effcurve->second;
                    }
                    auto effcurveFine = cont->triggerefficienciesFine.find(trg);
                    if(effcurveFine != cont->triggerefficienciesFine.end()) {
                        std::cout << "Found trigger efficiency (fine) for trigger " << trg << " and radius " << double(R)/10. << std::endl;
                        triggereffFine = effcurveFine->second;
                    }
                }
            }

            writer->cd(outdirname.data());
            if(triggereff) triggereff->Write();
            if(triggereffFine) triggereffFine->Write();

            for(auto observable : observables) {
                std::string histname = Form("h%sVsPtWeighted", observable.data());
                auto rawobject = histlist->FindObject(histname.data());
                TH2 *rawhist(nullptr);
                if((rawobject->IsA() == THnSparse::Class()) || rawobject->InheritsFrom(THnSparse::Class())) {
                    std::cout << "Extracting raw data from THnSparse" << std::endl;
                    rawhist = makeRawHistFromSparse(static_cast<THnSparse *>(rawobject), triggerclusters[trg]);
                } else {
                    std::cout << "Using old style TH2 raw object" << std::endl;
                    rawhist = static_cast<TH2 *>(rawobject);
                }
                rawhist->SetDirectory(nullptr);
                rawhist->SetNameTitle(Form("h%sVsPt%sRaw", observable.data(), trg.data()), Form("%s vs. Pt for trigger %s (raw)", observable.data(), trg.data()));
                rawhist->Scale(weight);

                // Extract 1D projections and trigger efficiencies
                auto spec1Dnorebin = rawhist->ProjectionY(Form("jetSpectrumNoCorrNoRebin%s_%s_%s", outdirname.data(), trg.data(), observable.data()));
                spec1Dnorebin->SetDirectory(nullptr);
                spec1Dnorebin->Write();
                auto spec1Drebin = spec1Dnorebin->Rebin(ptbinning.size() -1, Form("jetSpectrumNoCorrRebin%s_%s_%s", outdirname.data(), trg.data(), observable.data()), ptbinning.data());
                spec1Drebin->SetDirectory(nullptr);
                spec1Drebin->Scale(1., "width");
                spec1Drebin->Write();

                // correct for the trigger efficiencies
                if(triggereff) {
                    auto spec1DnorebinCorrected = static_cast<TH1 *>(spec1Dnorebin->Clone(Form("jetSpectrumCorrectedNoRebin%s_%s_%s", outdirname.data(), trg.data(), observable.data())));
                    spec1DnorebinCorrected->SetDirectory(nullptr);
                    spec1DnorebinCorrected->Divide(triggereffFine);
                    spec1DnorebinCorrected->Write();
                }
                if(triggereffFine) {
                    auto spec1DrebinCorrected = static_cast<TH1 *>(spec1Drebin->Clone(Form("jetSpectrumCorrectedRebin%s_%s_%s", outdirname.data(), trg.data(), observable.data())));
                    spec1DrebinCorrected->SetDirectory(nullptr);
                    spec1DrebinCorrected->Divide(triggereff);
                    spec1DrebinCorrected->Write();
                }

                std::vector<double> obsbinning;
                obsbinning.emplace_back(rawhist->GetXaxis()->GetBinLowEdge(1));
                for(auto bin : ROOT::TSeqI(0, rawhist->GetXaxis()->GetNbins())){
                    double step = rawhist->GetXaxis()->GetBinUpEdge(bin+1);
                    obsbinning.emplace_back(step);
                } 

                auto resulthist = new TH2D(Form("h%sVsPt%sCorrected", observable.data(), trg.data()), Form("%s vs. Pt for trigger %s (corrected)", observable.data(), trg.data()), obsbinning.size()-1, obsbinning.data(), ptbinning.size()-1, ptbinning.data());
                resulthist->SetDirectory(nullptr);

                for(auto ipt : ROOT::TSeqI(0, resulthist->GetYaxis()->GetNbins())) {
                    auto effval = triggereff ? 1./triggereff->GetBinContent(ipt+1) : 1.;
                    std::unique_ptr<TH1> slice(rawhist->ProjectionX("slice", rawhist->GetYaxis()->FindBin(resulthist->GetYaxis()->GetBinLowEdge(ipt+1) + kVerySmall), rawhist->GetYaxis()->FindBin(resulthist->GetYaxis()->GetBinUpEdge(ipt+1) - kVerySmall)));
                    slice->Scale(effval);
                    for(auto iobs : ROOT::TSeqI(0, resulthist->GetXaxis()->GetNbins())) { 
                        resulthist->SetBinContent(iobs+1, ipt+1, slice->GetBinContent(iobs+1));
                        resulthist->SetBinError(iobs+1, ipt+1, slice->GetBinError(iobs+1));
                    }
                }

                rawhist->Write();
                resulthist->Write();
            }
        }
    }
}