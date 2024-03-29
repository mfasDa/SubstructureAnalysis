#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/roounfold.C"
#include "../helpers/math.C"
#include "../helpers/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../helpers/pthard.C"
#include "../helpers/substructuretree.C"
#include "../helpers/unfolding.C"
#include "binnings/binningPt1D.C"

std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};

struct PtCorrectionHists {
    TH1 *rawspectrum;
    TH2 *ptcorrelation;
    TH2 *energycorrelation;
    TH2 *energycorrelationFine;
    TH2 *eleadcorrold;
    TH2 *eleadcorrnew;
    TH2 *deltaeresult;
};

TGraph * readInverseNonLinCorr(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open("/data1/mfasel/Fulljets/pp_13TeV/Substructuretree/data_mc/NonLinCorr/nonLinCorrInverted.root"));
    gROOT->cd();
    auto corr = static_cast<TGraph *>(reader->Get("invertedkTestbeamv3"));
    return corr;
}

TH1 *readSmearedMC(const std::string_view inputfile){
    auto binning = getJetPtBinningNonLinSmearLarge();
    ROOT::RDataFrame df(GetNameJetSubstructureTree(inputfile), inputfile);
    TH1 *result(nullptr);
    auto hist = df.Filter([](double ptsim, int ptbin) { return !IsOutlierFast(ptsim, ptbin); },{"PtJetSim", "PtHardBin"}).Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec", "PythiaWeight");
    result = histcopy(hist.GetPtr());
    result->SetDirectory(nullptr);
    return result;
}

PtCorrectionHists readSmearedData(const std::string_view inputfile, bool downscaleweighted){
    TGraph *oldcorrection = readInverseNonLinCorr("");
    std::array<double, 7> fNonLinearityParams ={{0.9892, 0.1976, 0.865, 0.06775, 156.6, 47.18, 0.97}};
    auto lambdacorr = [fNonLinearityParams](double *x, double *p) { return fNonLinearityParams[6]/(fNonLinearityParams[0]*(1./(1.+fNonLinearityParams[1]*exp(-x[0]/fNonLinearityParams[2]))*1./(1.+fNonLinearityParams[3]*exp((x[0]-fNonLinearityParams[4])/fNonLinearityParams[5])))); };
    TF1 newcorrection("newcorrection", lambdacorr, 0., 200., 1);
    auto binning = getJetPtBinningNonLinSmearLarge();
    ROOT::RDataFrame df(GetNameJetSubstructureTree(inputfile), inputfile);
    auto corrframe = df.Define("Theta", [](double eta) { return EtaToTheta(eta);}, {"EtaRec"})
                       .Define("EleadOld", "EJetRec * ZLeadingNeutralRec")
                       .Define("Eleadraw", [oldcorrection] (double eold) { return eold * oldcorrection->Eval(eold); }, {"EleadOld"})
                       .Define("EleadingCorr", [newcorrection] (double energy) { return energy * newcorrection.Eval(energy); }, {"Eleadraw"})
                       .Define("DeltaELead", "EleadOld - EleadingCorr")
                       .Define("EJetCorr", "EJetRec - DeltaELead")
                       .Define("PtJetRecCorr", [](double ejetcorr, double theta){ return TMath::Sin(theta) * ejetcorr; }, {"EJetCorr", "Theta"});
                       /*
                       .Define("EJetCorr", [](double ejet, double eleadold, double eleadcorr, double eta, double theta, double phi) { 
                                TLorentzVector jetvecold, jetvecnew, clustervecOld, clustervecNew;
                                jetvecold.SetPtEtaPhiE(TMath::Sin(theta) * ejet, eta, phi, ejet);
                                clustervecOld.SetPtEtaPhiE(TMath::Sin(theta) * eleadold, eta, phi, eleadold);
                                clustervecNew.SetPtEtaPhiE(TMath::Sin(theta) * eleadcorr, eta, phi, eleadcorr);
                                jetvecnew = jetvecold - clustervecOld + clustervecNew;
                                return jetvecnew.E();
                           }, 
                           {"EJetRec", "EleadOld", "EleadingCorr", "EtaRec", "Theta", "PhiRec"})
                       
                       */
    ROOT::RDF::RResultPtr<TH1D> hist;
    if(downscaleweighted){
        // data - no outlier rejection
        hist = corrframe.Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRecCorr", "EventWeight");
    } else {
        hist = corrframe.Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRecCorr");
    }
    auto correlationhist = corrframe.Histo2D({"correlationRawCorr", "Pt correlation", static_cast<int>(binning.size()-1), binning.data(), static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec", "PtJetRecCorr");
    auto energycorrelationhist = corrframe.Histo2D({"energycorrelationRawCorr", "E correlation", static_cast<int>(binning.size()-1), binning.data(), static_cast<int>(binning.size()-1), binning.data()}, "EJetRec", "EJetCorr");
    auto energycorrelationhistFine = corrframe.Histo2D({"energycorrelationRawCorrFine", "E correlation", 200, 0., 200., 200, 0., 200.}, "EJetRec", "EJetCorr");
    auto leadcorrelationhistOld = corrframe.Histo2D({"EleadCorrOld", "Leading E correlation", 200, 0., 200., 200, 0., 200.}, "Eleadraw", "EleadOld");
    auto leadcorrelationhistNew = corrframe.Histo2D({"EleadCorrNew", "Leading E correlation", 200, 0., 200., 200, 0., 200.}, "Eleadraw", "EleadingCorr");
    auto leadcorrelationhistOld = corrframe.Histo2D({"EleadCorrOldNew", "Leading E correlation", 200, 0., 200., 200, 0., 200.}, "EleadOld", "EleadNew");
    auto deltaehist = corrframe.Histo2D({"DeltaEleadHist", "Delta E Leading", 200, 0., 200., 400, -20., 20.}, "EJetRec", "DeltaELead");
    auto result = histcopy(hist.GetPtr());
    result->SetDirectory(nullptr);
    auto rescorr = static_cast<TH2 *>(histcopy(correlationhist.GetPtr()));
    rescorr->SetDirectory(nullptr);
    auto escorr = static_cast<TH2 *>(histcopy(energycorrelationhist.GetPtr()));
    escorr->SetDirectory(nullptr);
    auto escorrfine = static_cast<TH2 *>(histcopy(energycorrelationhistFine.GetPtr()));
    escorrfine->SetDirectory(nullptr);
    auto eleadOldcorrfine = static_cast<TH2 *>(histcopy(leadcorrelationhistOld.GetPtr()));
    eleadOldcorrfine->SetDirectory(nullptr);
    auto eleadNewcorrfine = static_cast<TH2 *>(histcopy(leadcorrelationhistNew.GetPtr()));
    eleadNewcorrfine->SetDirectory(nullptr);
    auto deltaeresult = static_cast<TH2 *>(histcopy(deltaehist.GetPtr()));
    deltaeresult->SetDirectory(nullptr);
    return {result, rescorr, escorr, escorrfine, eleadOldcorrfine, eleadNewcorrfine, deltaeresult};
}

std::vector<TH1 *> extractCENTNOTRDCorrection(std::string_view filename){
    auto binning = getJetPtBinningNonLinSmearLarge();
    ROOT::RDataFrame df(GetNameJetSubstructureTree(filename), filename);
    TH1 *result(nullptr);
    auto selCENT = df.Filter("TriggerClusterIndex < 1");
    auto speccentnotrd = df.Histo1D({"speccentnotrd", "Spectrum centnotrd", static_cast<int>(binning.size()) - 1, binning.data()}, "PtJetRec"),
         speccent = selCENT.Histo1D({"speccent", "Spectrum cent", static_cast<int>(binning.size()) - 1, binning.data()}, "PtJetRec");
    speccent->Sumw2();
    speccentnotrd->Sumw2();
    auto correction = histcopy(speccentnotrd.GetPtr());
    correction->SetNameTitle("CENTNOTRDCorrection", "Correction for the unmeasured CENTNOTRD Luminosity");
    correction->SetDirectory(nullptr);
    correction->Divide(speccentnotrd.GetPtr(), speccent.GetPtr(), 1., 1., "b");
    auto rescentnotrd = histcopy(speccentnotrd.GetPtr()), rescent = histcopy(speccent.GetPtr());
    rescentnotrd->SetDirectory(nullptr);
    rescent->SetDirectory(nullptr);
    return {rescentnotrd, rescent, correction};
}

double extractCENTNOTRDCorrectionFromClusterCounter(const std::string_view filename, double radius) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd(Form("JetSubstructure_FullJets_R%02d_EJ1", int(radius*10.)));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto clustercounter = static_cast<TH1 *>(histlist->FindObject("hTriggerClusterCounter"));
    if(!clustercounter) return -1.;             // not found
    auto centpluscentnotrdcounter = clustercounter->GetBinContent(clustercounter->FindBin(0)),
         onlycentnotrdcounter = clustercounter->GetBinContent(clustercounter->FindBin(2));
    return (centpluscentnotrdcounter + onlycentnotrdcounter) / centpluscentnotrdcounter;
}

double extractLumiCENT(const std::string_view filename){
    std::pair<double, double> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("JetSubstructure_FullJets_R02_INT7");
    auto list = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto lumihist = static_cast<TH1 *>(list->FindObject("hLumiMonitor"));
    auto lumicent = lumihist->GetBinContent(lumihist->GetXaxis()->FindBin("CENT"));
    return lumicent;
}

std::map<std::string, int> readNriggers(const std::string_view filename = "data/merged_1617/AnalysisResults_split.root"){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::map<std::string, int> nevents;
    for(auto trg : triggers) {
        std::stringstream dirname;
        dirname << "JetSubstructure_FullJets_R02_" << trg;
        reader->cd(dirname.str().data());
        auto list = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto evhist = static_cast<TH1 *>(list->FindObject("hEventCounter"));
        nevents[trg] = evhist->GetBinContent(1);
    }    
    return nevents;
}

std::vector<std::string> getSortedKeys(const std::map<std::string, std::vector<TObject *>> &data) {
    std::vector<std::string> keys;
    for(const auto &k : data){
        keys.push_back(k.first);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

void checkRawNonLinCorr(double radius, const std::string_view indatadir = ""){
    ROOT::EnableImplicitMT(8);
    std::string datadir;
    if (indatadir.length()) datadir = std::string(indatadir);
    else datadir = gSystem->GetWorkingDirectory();
    std::cout << "[Bayes unfolding] Using data directory " << datadir << std::endl;
    std::cout << "[Bayes unfolding] Reading luminosity for cluster CENT " << std::endl;
    std::string normfilename = Form("%s/data/merged_17/AnalysisResults_split.root", datadir.data());
    auto lumiCENT = extractLumiCENT(normfilename.data());
    std::cout << "[Bayes unfolding] Getting correction factor for CENTNOTRD cluster" << std::endl;
    std::vector<TH1 *> centnotrdCorrection;
    double cntcorrectionvalue = extractCENTNOTRDCorrectionFromClusterCounter(normfilename.data(), radius);
    if(cntcorrectionvalue < 0){
        // counter historgam not found (old output) - try with jet spectra
        std::cout << "[Bayes unfolding] Getting CENTNOTRD correction from spectra comparison (old method)" << std::endl;
        centnotrdCorrection = extractCENTNOTRDCorrection(Form("%s/data/merged_17/JetSubstructureTree_FullJets_R%02d_EJ1.root", datadir.data(), int(radius*10.)));
        TF1 fit("centnotrdcorrfit", "pol0", 0., 200.);
        centnotrdCorrection[2]->Fit(&fit, "N", "", 20., 200.);
        cntcorrectionvalue = fit.GetParameter(0);
    } else {
        std::cout << "[Bayes unfolding] CENTNOTRD correction was obtained from trigger cluster counter (new method)" << std::endl;
    }
    std::cout << "[Bayes unfolding] Using CENTNOTRD correction factor " << cntcorrectionvalue << std::endl;
    auto lumiCENTNOTRD = lumiCENT * cntcorrectionvalue;
    auto lumihist = new TH1D("luminosities", "Luminosities", 3, 0., 3.);
    lumihist->SetDirectory(nullptr);
    lumihist->GetXaxis()->SetBinLabel(1, "INT7");
    lumihist->GetXaxis()->SetBinLabel(2, "CENT");
    lumihist->GetXaxis()->SetBinLabel(3, "CENTNOTRD");
    lumihist->SetBinContent(2, lumiCENT);
    lumihist->SetBinContent(3, lumiCENTNOTRD);
    std::map<std::string, TH1 *> mcspectra, dataspectra;
    std::map<std::string, TH2 *> ptcorrelationhists, energycorrelationhists, 
                                 energycorrelationhistsFine, eleadoldcorrelationhists, 
                                 eleadnewcorrelationhists, deltaehists;
    // Read MC specta
    std::cout << "[Bayes unfolding] Reading Monte-Carlo spectra for trigger efficiency correction" << std::endl;
    for(const auto &trg : triggers) {
        std::stringstream filename;
        filename << datadir << "/mc/merged_calo/JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(radius*10.) << "_" << trg << "_merged.root";
        auto spec = readSmearedMC(filename.str());
        spec->SetName(Form("mcspec_R%02d_%s", int(radius*10.), trg.data()));
        mcspectra[trg] = spec;
    }
    // Read data specta
    std::cout << "[Bayes unfolding] Reading data spectra for all triggers" << std::endl;
    for(const auto &trg : triggers) {
        std::stringstream filename;
        filename << datadir << "/data/" << (trg == "INT7" ? "merged_1617" : "merged_17") << "/JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(radius*10.) << "_" << trg << ".root";
        auto spectre = readSmearedData(filename.str(), trg == "EJ2");
        spectre.rawspectrum->SetName(Form("dataspec_R%02d_%s", int(radius*10.), trg.data()));
        dataspectra[trg] = spectre.rawspectrum;
        spectre.ptcorrelation->SetName(Form("ptcorr_R%02d_%s", int(radius*10.), trg.data()));
        ptcorrelationhists[trg] = spectre.ptcorrelation;
        spectre.energycorrelation->SetName(Form("Ecorr_R%02d_%s", int(radius*10.), trg.data()));
        energycorrelationhists[trg] = spectre.energycorrelation;
        spectre.energycorrelationFine->SetName(Form("EcorrFine_R%02d_%s", int(radius*10.), trg.data()));
        energycorrelationhistsFine[trg] = spectre.energycorrelationFine;
        spectre.eleadcorrold->SetName(Form("EleadOld_R%02d_%s", int(radius*10.), trg.data()));
        eleadoldcorrelationhists[trg] = spectre.eleadcorrold;
        spectre.eleadcorrnew->SetName(Form("EleadNew_R%02d_%s", int(radius*10.), trg.data()));
        eleadnewcorrelationhists[trg] = spectre.eleadcorrnew;
        spectre.deltaeresult->SetName(Form("DeltaEleading_R%02d_%s", int(radius*10.), trg.data()));
        deltaehists[trg] = spectre.deltaeresult;
    }
    // get weights and renormalize data spectra
    std::cout << "[Bayes unfolding] normalize spectra" << std::endl;
    auto weights = readNriggers(Form("%s/data/merged_1617/AnalysisResults_split.root", datadir.data()));
    std::map<std::string, TH1 *> hnorm;
    for(auto &spec : dataspectra) {
        auto trgweight = weights.find(spec.first)->second;
        if(spec.first == "EJ1") {
            spec.second->Scale(1./lumiCENTNOTRD);
        } else if(spec.first == "EJ2") {
            spec.second->Scale(1./lumiCENT);
        } else {
            lumihist->SetBinContent(1, trgweight);
            spec.second->Scale(1./trgweight);
        }
        auto normhist = new TH1D(Form("norm%s", spec.first.data()), Form("event count trigger %s", spec.first.data()), 1, 0.5, 1.5);
        normhist->SetBinContent(1, trgweight);
        hnorm[spec.first] = normhist;
    }
    // build efficiencies, correct triggered spectra
    std::cout << "[Bayes unfolding] Building trigger efficiency" << std::endl;
    std::map<std::string, TH1 *> efficiencies;
    auto reference = mcspectra.find("INT7")->second;
    for(auto &trg : triggers) {
        if(trg == "INT7") continue;
        auto eff = histcopy(mcspectra.find(trg)->second);
        eff->SetName(Form("Efficiency_R%02d_%s", int(radius*10.), trg.data()));
        eff->Divide(eff, reference, 1., 1., "b");
        efficiencies[trg] = eff;
        auto tocorrect = dataspectra.find(trg)->second;
        tocorrect->Divide(eff);
    }

    // ratio trigger / min bias
    std::map<std::string, TH1 *> ratios;
    auto dataref = dataspectra.find("INT7")->second;
    for(auto &trg : triggers){
        if(trg == "INT7") continue;
        auto ratio = histcopy(dataspectra.find(trg)->second);
        ratio->SetName(Form("%soverMB_R%02d", trg.data(), int(radius*10)));
        ratio->Divide(dataref);
        ratios[trg] = ratio;
    }

    // combine jet spectrum in data (for unfolding)
    auto hraw = histcopy(dataspectra.find("INT7")->second);
    hraw->SetNameTitle("hraw", "raw spectrum from various triggers");
    auto triggered = dataspectra.find("EJ1")->second;
    for(auto b : ROOT::TSeqI(0, hraw->GetNbinsX())){
        if(hraw->GetXaxis()->GetBinCenter(b+1) < 70.) continue;       // Use data from INT7 trigger
        // else Use data from EJ1 trigger
        hraw->SetBinContent(b+1, triggered->GetBinContent(b+1));
        hraw->SetBinError(b+1, triggered->GetBinError(b+1));
    }
    std::cout << "[Bayes unfolding] Raw spectrum ready, getting detector response ..." << std::endl;


    // write everything
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[Bayes unfolding] Writeing output" << std::endl;
    std::unique_ptr<TFile> writer(TFile::Open(Form("rawEffectNonLinCorr_R%02d.root", int(radius*10.)), "RECREATE"));
    writer->mkdir("rawlevel");
    writer->cd("rawlevel");
    hraw->Write();
    lumihist->Write();
    for(auto m : mcspectra) {normalizeBinWidth(m.second); m.second->Write();}
    for(auto d : dataspectra) {normalizeBinWidth(d.second); d.second->Write();}
    for(auto p : ptcorrelationhists) p.second->Write();
    for(auto ec : energycorrelationhists) ec.second->Write();
    for(auto ec : energycorrelationhistsFine) ec.second->Write();
    for(auto ec : eleadoldcorrelationhists) ec.second->Write();
    for(auto ec : eleadnewcorrelationhists) ec.second->Write();
    for(auto d : deltaehists) d.second->Write();
    for(auto e : efficiencies) e.second->Write();
    for(auto n : hnorm) n.second->Write();
    for(auto r : ratios) r.second->Write();
    for(auto c : centnotrdCorrection) c->Write();
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[Bayes unfolding] All done" << std::endl;
    std::cout << "======================================================================\n";
}