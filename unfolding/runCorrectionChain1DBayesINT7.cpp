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

std::vector<std::string> triggers = {"INT7", "EJ1"};

TH1 *readSmeared(const std::string_view inputfile, bool weighted, bool dooutlierrejection){
    auto binning = getJetPtBinningNonLinSmearLarge();
    ROOT::RDataFrame df(GetNameJetSubstructureTree(inputfile), inputfile);
    TH1 *result(nullptr);
    if(weighted){
        if(dooutlierrejection){
            auto hist = df.Filter([](double ptsim, int ptbin) { return !IsOutlierFast(ptsim, ptbin); },{"PtJetSim", "PtHardBin"}).Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec", "PythiaWeight");
            result = histcopy(hist.GetPtr());
        } else {
            auto hist = df.Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec", "PythiaWeight");
            result = histcopy(hist.GetPtr());
        }
    } else {
        if(dooutlierrejection) {
            auto hist = df.Filter([](double ptsim, int ptbin) { return !IsOutlierFast(ptsim, ptbin); },{"PtJetSim", "PtHardBin"}).Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec");
            result = histcopy(hist.GetPtr());
        } else {
            auto hist = df.Histo1D({"spectrum", "spectrum", static_cast<int>(binning.size()-1), binning.data()}, "PtJetRec");
            result = histcopy(hist.GetPtr());
        }
    }
    result->SetDirectory(nullptr);
    return result;
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

void runCorrectionChain1DBayesINT7(double radius, const std::string_view indatadir = ""){
    std::string datadir;
    if (indatadir.length()) datadir = std::string(indatadir);
    else datadir = gSystem->GetWorkingDirectory();
    std::cout << "[Bayes unfolding] Using data directory " << datadir << std::endl;
    // Read data specta
    std::cout << "[Bayes unfolding] Reading data spectra for INT7" << std::endl;
    std::stringstream filename;
    filename << datadir << "/data/merged_1617/JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(radius*10.) << "_INT7.root";
    auto hraw = readSmeared(filename.str(), false, false);
    hraw->SetNameTitle("hraw", "raw spectrum from various triggers");
    // get weights and renormalize data spectra
    std::cout << "[Bayes unfolding] normalize spectra" << std::endl;
    auto weights = readNriggers(Form("%s/data/merged_1617/AnalysisResults_split.root", datadir.data()));
    auto trgweight = weights.find("INT7")->second;
    hraw->Scale(1./trgweight);
    std::cout << "[Bayes unfolding] Raw spectrum ready, getting detector response ..." << std::endl;

    // read MC
    auto binningdet = getJetPtBinningNonLinSmearLarge(), 
         binningpart = getJetPtBinningNonLinTrueLarge();
    auto ptmin = *(binningdet.begin()), ptmax = *(binningdet.rbegin());
    TH1 *htrue = new TH1D("htrue", "true spectrum", binningpart.size()-1, binningpart.data()),
        *hsmeared = new TH1D("hsmeared", "det mc", binningdet.size()-1, binningdet.data()), 
        *hsmearedClosure = new TH1D("hsmearedClosure", "det mc (for closure test)", binningdet.size() - 1, binningdet.data()),
        *htrueClosure = new TH1D("htrueClosure", "true spectrum (for closure test)", binningpart.size() - 1, binningpart.data()),
        *htrueFull = new TH1D("htrueFull", "non-truncated true spectrum", binningpart.size() - 1, binningpart.data()),
        *htrueFullClosure = new TH1D("htrueFullClosure", "non-truncated true spectrum (for closure test)", binningpart.size() - 1, binningpart.data()),
        *hpriorsClosure = new TH1D("hpriorsClosure", "non-truncated true spectrum (for closure test, same jets as repsonse matrix)", binningpart.size() - 1, binningpart.data());
    TH2 *responseMatrix = new TH2D("responseMatrix", "response matrix", binningdet.size()-1, binningdet.data(), binningpart.size()-1, binningpart.data()),
        *responseMatrixClosure = new TH2D("responseMatrixClosure", "response matrix (for closure test)", binningdet.size()-1, binningdet.data(), binningpart.size()-1, binningpart.data());
  
    {
        TRandom closuresplit;
        std::stringstream filemc;
        filemc << datadir << "/mc/merged_calo/JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(radius*10.) << "_INT7_merged.root";
        std::unique_ptr<TFile> fread(TFile::Open(filemc.str().data(), "READ"));
        TTreeReader mcreader(GetDataTree(*fread));
        TTreeReaderValue<double>  ptrec(mcreader, "PtJetRec"), 
                                  ptsim(mcreader, "PtJetSim"), 
                                  nefrec(mcreader, "NEFRec"),
                                  weight(mcreader, "PythiaWeight");
        TTreeReaderValue<int>     pthardbin(mcreader, "PtHardBin");
        bool closureUseSpectrum;
        for(auto en : mcreader){
            if(IsOutlierFast(*ptsim, *pthardbin)) continue;
            double rdm = closuresplit.Uniform();
            closureUseSpectrum = (rdm < 0.2);
            htrueFull->Fill(*ptsim, *weight);
            if(closureUseSpectrum) htrueFullClosure->Fill(*ptsim, *weight);
            else hpriorsClosure->Fill(*ptsim, *weight);
            if(*ptrec > ptmin && *ptrec < ptmax){
                htrue->Fill(*ptsim, *weight);
                hsmeared->Fill(*ptrec, *weight);
                responseMatrix->Fill(*ptrec, *ptsim, *weight);

                if(closureUseSpectrum) {
                    hsmearedClosure->Fill(*ptrec, *weight);
                    htrueClosure->Fill(*ptsim, *weight);
                } else {
                    responseMatrixClosure->Fill(*ptrec, *ptsim, *weight);
                }
            }
        }
    }

    // Calculate kinematic efficiency
    std::cout << "[Bayes unfolding] Make kinematic efficiecny for raw unfolding ..." << std::endl;
    auto effKine = histcopy(htrue);
    effKine->SetDirectory(nullptr);
    effKine->SetName("effKine");
    effKine->Divide(effKine, htrueFull, 1., 1., "b");

    std::cout << "[Bayes unfolding] ... and for closure test" << std::endl;
    auto effKineClosure = histcopy(htrueClosure);
    effKineClosure->SetDirectory(nullptr);
    effKineClosure->SetName("effKineClosure");
    effKineClosure->Divide(htrueFullClosure);

    std::cout << "[Bayes unfolding] Building RooUnfold response" << std::endl;
    RooUnfoldResponse response(nullptr, htrueFull, responseMatrix), responseClosure(nullptr, hpriorsClosure, responseMatrixClosure);

    std::cout << "Running unfolding" << std::endl;
    std::map<std::string, std::vector<TObject *>> iterresults;
    RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovariance;
    const double kSizeEmcalPhi = 1.88,
                 kSizeEmcalEta = 1.4;
    double acceptance = (kSizeEmcalPhi - 2 * radius) * (kSizeEmcalEta - 2 * radius) / (TMath::TwoPi());
    double crosssection = 57.8;
    double epsilon_vtx = 0.8228; // for the moment hard coded, for future analyses determined automatically from the output
    for(auto iter : ROOT::TSeqI(1, 36)){
        std::cout << "[Bayes unfolding] Doing iteration " << iter << "\n================================================================\n";
        std::cout << "[Bayes unfolding] Running unfolding" << std::endl;
        RooUnfoldBayes unfolder(&response, hraw, iter);
        auto unfolded = unfolder.Hreco(errorTreatment);
        unfolded->SetName(Form("unfolded_iter%d", iter));
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "[Bayes unfolding] Running MC closure test" << std::endl;
        RooUnfoldBayes unfolderClosure(&responseClosure, hsmearedClosure);
        auto unfoldedClosure = unfolderClosure.Hreco(errorTreatment);
        unfoldedClosure->SetName(Form("unfoldedClosure_iter%d", iter));

        // back-folding test
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "[Bayes unfolding] Running refolding test" << std::endl;
        auto backfolded = MakeRefolded1D(hraw, unfolded, response);
        backfolded->SetName(Form("backfolded_iter%d", iter));
        auto backfoldedClosure = MakeRefolded1D(hsmearedClosure, unfoldedClosure, responseClosure);
        backfoldedClosure->SetName(Form("backfoldedClosure_iter%d", iter));

        // normalize spectrum (but write as new object)
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "[Bayes unfolding] Normalizing spectrum" << std::endl;
        auto normalized = histcopy(unfolded);
        normalized->SetNameTitle(Form("normalized_iter%d", iter), Form("Normalized for regularization %d", iter));
        normalized->Scale(crosssection*epsilon_vtx/acceptance);
        normalizeBinWidth(normalized);

        // preparing for output finding
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "[Bayes unfolding] Building output list" << std::endl;
        iterresults[Form("iteration%d", iter)] = {unfolded, normalized, backfolded, unfoldedClosure, backfoldedClosure};
        std::cout << "----------------------------------------------------------------------\n";
        std::cout << "[Bayes unfolding] regularization done" << std::endl;
        std::cout << "======================================================================\n";
    }

    // write everything
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[Bayes unfolding] Writeing output" << std::endl;
    std::unique_ptr<TFile> writer(TFile::Open(Form("corrected1DBayes_R%02d.root", int(radius*10.)), "RECREATE"));
    writer->mkdir("rawlevel");
    writer->cd("rawlevel");
    hraw->Write();
    writer->mkdir("detectorresponse");
    writer->cd("detectorresponse");
    htrueFull->Write();
    htrueFullClosure->Write();
    htrue->Write();
    htrueClosure->Write();
    hpriorsClosure->Write();
    hsmeared->Write();
    hsmearedClosure->Write();
    responseMatrix->Write();
    responseMatrixClosure->Write();
    hraw->Write();
    effKine->Write();
    effKineClosure->Write();
    for(const auto &k : getSortedKeys(iterresults)) {
        writer->mkdir(k.data());
        writer->cd(k.data());
        for(auto h : iterresults.find(k)->second) h->Write();
    }
    std::cout << "----------------------------------------------------------------------\n";
    std::cout << "[Bayes unfolding] All done" << std::endl;
    std::cout << "======================================================================\n";
}