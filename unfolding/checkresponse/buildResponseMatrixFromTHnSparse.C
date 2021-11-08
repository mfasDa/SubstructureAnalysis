#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/roounfold.C"
#include "../../helpers/math.C"
#include "../binnings/binningPt2D.C"

std::vector<double> makeObservableBinning(const TAxis *obsaxis, double obsmax = DBL_MAX) {
    std::vector<double> obsbinning;
    obsbinning.emplace_back(obsaxis->GetBinLowEdge(1)); 
    for(auto bin : ROOT::TSeqI(0, obsaxis->GetNbins())) {
        auto step = obsaxis->GetBinUpEdge(bin+1);
        if(step > obsmax) continue;
        obsbinning.emplace_back(step);
    } 
    return obsbinning;
}

RooUnfoldResponse *makeResponse(THnSparse *responsedata, std::vector<double> &detptbinning, std::vector<double> &partptbinning, double obsmax = DBL_MAX, bool verbose = false) {
    std::vector<double> detobsbinning = makeObservableBinning(responsedata->GetAxis(0), obsmax), 
                        partobsbinning = makeObservableBinning(responsedata->GetAxis(2), obsmax);
    TH2F dettemplate("dettemplate", "det template", detobsbinning.size() - 1, detobsbinning.data(), detptbinning.size() - 1, detptbinning.data()),
         parttemplate("parttemplate", "part template", partobsbinning.size() - 1, partobsbinning.data(), partptbinning.size() - 1, partptbinning.data());

    auto &ptpartmin = partptbinning.front(),
         &ptpartmax = partptbinning.back(),
         &ptdetmin = detptbinning.front(),
         &ptdetmax = detptbinning.back(),
         &obspartmin = partobsbinning.front(),
         &obspartmax = partobsbinning.back(),
         &obsdetmin = detobsbinning.front(),
         &obsdetmax = detobsbinning.back();
    std::cout << "Using part. min. pt " << ptpartmin << " and max " << ptpartmax << std::endl;
    std::cout << "Using det. min. pt " << ptdetmin << " and max " << ptdetmax << std::endl;

    auto resultresponse = new RooUnfoldResponse(Form("%s_rebinned", responsedata->GetName()), Form("%s rebinned", responsedata->GetTitle()));
    resultresponse->UseOverflow(false);
    resultresponse->Setup(&dettemplate, &parttemplate);

    Int_t coordinate[4];
    for(auto ib : ROOT::TSeqI(0, responsedata->GetNbins())) {
        auto weight = responsedata->GetBinContent(ib, coordinate);
        double zgdet = responsedata->GetAxis(0)->GetBinCenter(coordinate[0]),
               ptdet = responsedata->GetAxis(1)->GetBinCenter(coordinate[1]),
               zgpart = responsedata->GetAxis(2)->GetBinCenter(coordinate[2]),
               ptpart = responsedata->GetAxis(3)->GetBinCenter(coordinate[3]);
        // check whether pt is within det-level and part-level range
        if(ptdet < ptdetmin || ptdet > ptdetmax || ptpart < ptpartmin || ptpart > ptpartmax) {
            if(verbose) std::cout << "Point (" << ptpart << " part - " << ptdet << " det) outside range" << std::endl;
            continue;
        }
        if(zgdet < obsdetmin || zgdet > obsdetmax || zgpart < obspartmin || zgpart > obspartmax) {
            if(verbose) std::cout << "Point (" << zgpart << " part - " << zgdet << " det) outside range in SD" << std::endl;
            continue;
        }
        resultresponse->Fill(zgdet, ptdet, zgpart, ptpart, weight);
    }
    return resultresponse;
}

TH2 *makeRebinnedPt(const TH2 *inputspectrum, std::vector<double> partptbinning, double obsmax = DBL_MAX) {
    auto partaxis = inputspectrum->GetXaxis();
    std::vector<double> partobsbinning;
    partobsbinning.emplace_back(partaxis->GetBinLowEdge(1)); 
    for(auto bin : ROOT::TSeqI(0, partaxis->GetNbins())) {
        if(partaxis->GetBinCenter(bin+1) < obsmax) {
            partobsbinning.emplace_back(partaxis->GetBinUpEdge(bin+1));
        } else {
            break;
        }
    }
    
    return makeRebinned2D(inputspectrum, partobsbinning, partptbinning);
}

TH2 *extractKinematicEfficiency(THnSparse *responsedata, std::vector<double> & partptbinning, double detptmin, double detptmax, double obsmax = DBL_MAX) {
    std::cout << "Extracting kinematic efficiency for det. pt range from " << detptmin << " GeV/c to " << detptmax << " GeV/c" << std::endl;
    std::vector<double> partobsbinning = makeObservableBinning(responsedata->GetAxis(2), obsmax);

    std::unique_ptr<TH2> truefull(responsedata->Projection(3, 2, "e"));
    truefull->SetName("truefull");
    responsedata->GetAxis(1)->SetRangeUser(detptmin, detptmax);
    std::unique_ptr<TH2> truncated(responsedata->Projection(3, 2, "e"));
    truncated->SetName("truncated");
    responsedata->GetAxis(1)->SetRange(0, responsedata->GetAxis(1)->GetNbins()+1);

    TH2 *result = new TH2D("effKine", "Kinematic efficiency", partobsbinning.size()-1, partobsbinning.data(), partptbinning.size()-1, partptbinning.data());
    result->SetDirectory(nullptr);
    for(auto iptbin : ROOT::TSeqI(0, result->GetYaxis()->GetNbins())){
        double ptmin = result->GetYaxis()->GetBinLowEdge(iptbin+1),
               ptmax = result->GetYaxis()->GetBinUpEdge(iptbin+1);
        std::cout << "Doing bin " << iptbin << "(" << ptmin << " ... " << ptmax << ")" << std::endl;
        std::unique_ptr<TH1> slicefull(truefull->ProjectionX("slicefull", truefull->GetYaxis()->FindBin(ptmin), truefull->GetYaxis()->FindBin(ptmax), "e")),
                             slicetruncated(truncated->ProjectionX("slicetruncated", truncated->GetYaxis()->FindBin(ptmin), truncated->GetYaxis()->FindBin(ptmax), "e")); 
        slicetruncated->Divide(slicetruncated.get(), slicefull.get(), 1., 1., "b");
        for(auto iobsbin : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(iobsbin+1, iptbin+1, slicetruncated->GetBinContent(iobsbin+1));
            result->SetBinError(iobsbin+1, iptbin+1, slicetruncated->GetBinError(iobsbin+1));
        }
    }
    std::cout << "Finish projection " << std::endl;

    return result;
}

TH2 *extractEfficiencyPurity(THnSparse *inputhist, const std::vector<double> &ptbinning, double obsmax = DBL_MAX){
    // Axis 0 - observable
    // Axis 1 - pt
    // Axis 2 - tag status
    auto obsbinning = makeObservableBinning(inputhist->GetAxis(0), obsmax);
    auto histall = std::unique_ptr<TH2>(inputhist->Projection(1, 0, "e"));
    histall->SetName("histall");
    inputhist->GetAxis(2)->SetRange(4, 4);
    auto histtag = std::unique_ptr<TH2>(inputhist->Projection(1, 0, "e"));
    histtag->SetName("histtag");
    inputhist->GetAxis(2)->SetRange(0, inputhist->GetAxis(2)->GetNbins()+1);

    TH2 *result = new TH2D("effPure", "Kinematic efficiency or purity", obsbinning.size()-1, obsbinning.data(), ptbinning.size()-1, ptbinning.data());
    result->SetDirectory(nullptr);
    for(auto iptbin : ROOT::TSeqI(0, result->GetYaxis()->GetNbins())){
        double ptmin = result->GetYaxis()->GetBinLowEdge(iptbin+1),
               ptmax = result->GetYaxis()->GetBinUpEdge(iptbin+1);
        std::cout << "Doing bin " << iptbin << "(" << ptmin << " ... " << ptmax << ")" << std::endl;
        std::unique_ptr<TH1> sliceall(histall->ProjectionX("slicefull", histall->GetYaxis()->FindBin(ptmin), histall->GetYaxis()->FindBin(ptmax), "e")),
                             slicetag(histtag->ProjectionX("slicetag", histtag->GetYaxis()->FindBin(ptmin), histtag->GetYaxis()->FindBin(ptmax), "e")); 
        slicetag->Divide(slicetag.get(), sliceall.get(), 1., 1., "b");
        for(auto iobsbin : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
            result->SetBinContent(iobsbin+1, iptbin+1, slicetag->GetBinContent(iobsbin+1));
            result->SetBinError(iobsbin+1, iptbin+1, slicetag->GetBinError(iobsbin+1));
        }
    }
    return result;
}

TH2 *makeClosureTruthAll(TList *histos, const char *observable, const std::vector<double> &partptbinning, double obsmax) {
    auto closuresparse = static_cast<THnSparse *>(histos->FindObject(Form("h%sJetFindingEfficiencyClosureNoResp", observable)));
    if(!closuresparse) return nullptr;
    std::unique_ptr<TH2> projectionfine(closuresparse->Projection(1,0, "e"));
    auto projectionresult = makeRebinnedPt(projectionfine.get(), partptbinning, obsmax);
    projectionresult->SetDirectory(nullptr);
    return projectionresult;
}

TH2 *makeClosureDetAll(TList *histos, const char *observable, const std::vector<double> &detptbinning, double obsmax) {
    auto closuresparse = static_cast<THnSparse *>(histos->FindObject(Form("h%sJetFindingPurityClosureNoResp", observable)));
    if(!closuresparse) return nullptr;
    std::unique_ptr<TH2> projectionfine(closuresparse->Projection(1,0, "e"));
    auto projectionresult = makeRebinnedPt(projectionfine.get(), detptbinning, obsmax);
    projectionresult->SetDirectory(nullptr);
    return projectionresult;
}

void buildResponseMatrixFromTHnSparse(const char *filename = "AnalysisResults.root", const char *detbinningvar = "default", double maxnsd = 11., bool verbose = false) {
    std::vector<double> detptbinning = getDetPtBinning(detbinningvar),
                        partptbinning = {0, 10, 15, 20, 30, 40, 50, 60, 80, 100, 120, 140, 160, 180, 200, 240, 500};
    if(!detptbinning.size()) {
        std::cout << "Cannot create detector level pt binning for variation " << detbinningvar << std::endl;
        return;
    }

    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));

    std::vector<std::string> observables = {"Zg", "Rg", "Nsd", "Thetag"};
    std::map<int, TObjArray> objects;
    std::map<int, TObjArray> closureobjects;

    for(auto R : ROOT::TSeqI(2, 7)) {
        std::cout << "Doing jet radius R=" << (double(R)/10.) << std::endl;
        reader->cd(Form("SoftDropResponse_FullJets_R%02d_INT7", R));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        TObjArray outputobjects, cobjects;
        for(auto observable : observables) {
            double obsmax = DBL_MAX;
            if(observable == "Nsd") obsmax = maxnsd;
            std::cout << "Next observable " << observable << std::endl;
            THnSparse* responsedata = dynamic_cast<THnSparse *>(histlist->FindObject(Form("h%sResponseSparse", observable.data())));           
            std::cout << "Extracting response matrix for observable " << observable << std::endl;
            if(!responsedata) {
                std::cerr << "Did not find response matrix sparse for observable " << observable << std::endl;
                histlist->ls();
            }
            auto responsematrix = makeResponse(responsedata, detptbinning, partptbinning, obsmax, verbose);
            responsematrix->SetNameTitle(Form("Responsematrix%s_R%02d", observable.data(), R), Form("Response matrix for %s for R=%.1f", observable.data(), double(R)/10.));
            std::cout << "Extracting kinematic efficiency for observable " << observable << std::endl;
            auto effKine = extractKinematicEfficiency(responsedata, partptbinning, *detptbinning.begin(), *detptbinning.rbegin(), obsmax);
            effKine->SetNameTitle(Form("EffKine%s_R%02d", observable.data(), R), Form("Kinematic efficiency for %s for R=%.1f", observable.data(), double(R)/10.));
            outputobjects.Add(responsematrix);
            outputobjects.Add(effKine);

            // extract jetfinding efficiency and purity
            auto sparsePurity = static_cast<THnSparse *>(histlist->FindObject(Form("h%sJetFindingPurity", observable.data()))),
                 sparseEfficiency = static_cast<THnSparse *>(histlist->FindObject(Form("h%sJetFindingEfficiency", observable.data())));
            auto jetfindingeff = extractEfficiencyPurity(sparseEfficiency, partptbinning, obsmax),
                 jetfindingpurity = extractEfficiencyPurity(sparsePurity, detptbinning, obsmax);
            jetfindingeff->SetDirectory(nullptr);
            jetfindingeff->SetNameTitle(Form("JetFindingEff_%s_R%02d", observable.data(), R), Form("Jet finding efficiency for %s for R = %.1f", observable.data(), double(R)/10.));
            outputobjects.Add(jetfindingeff);
            jetfindingpurity->SetDirectory(nullptr);
            jetfindingpurity->SetNameTitle(Form("JetFindingPurity_%s_R%02d", observable.data(), R), Form("Jet finding purity for %s for R = %.1f", observable.data(), double(R)/10.));
            outputobjects.Add(jetfindingpurity);

            // Creating objects for the closure test
            auto closureresponsedata = dynamic_cast<THnSparse *>(histlist->FindObject(Form("h%sResponseClosureSparse", observable.data())));
            std::cout << "Extracting closure response matrix for observable " << observable << std::endl;
            if(!closureresponsedata) {
                std::cerr << "Did not find closure response matrix sparse for observable " << observable << std::endl;
                histlist->ls();
            }
            auto closureresponsematrix = makeResponse(closureresponsedata, detptbinning, partptbinning, obsmax, verbose);
            closureresponsematrix->SetNameTitle(Form("ResponsematrixClosure%s_R%02d", observable.data(), R), Form("Response matrix for closure test for %s for R=%.1f", observable.data(), double(R)/10.));
            std::cout << "Extracting closure kinematic efficiency for observable " << observable << std::endl;
            auto closureeffkine = extractKinematicEfficiency(closureresponsedata, partptbinning, *detptbinning.begin(), *detptbinning.rbegin(), obsmax);
            closureeffkine->SetNameTitle(Form("EffKineClosure%s_R%02d", observable.data(), R), Form("Kinematic efficiency for %s for R=%.1f", observable.data(), double(R)/10.));
            std::cout << "Extracting det. level spectrum and true spectrum (opposite sample)" << std::endl;
            auto closuretruth = makeRebinnedPt(static_cast<TH2 *>(histlist->FindObject(Form("h%sPartLevelClosureNoRespFine", observable.data()))), partptbinning, obsmax),
                 closuredet = makeRebinnedPt(static_cast<TH2 *>(histlist->FindObject(Form("h%sDetLevelClosureNoRespFine", observable.data()))), detptbinning, obsmax);
            closuretruth->SetDirectory(nullptr);
            closuredet->SetDirectory(nullptr);
            closuretruth->SetNameTitle(Form("closuretruth%s_R%02d", observable.data(), R), Form("Truth spectrum (closure test) for observable %s for R=%.1f", observable.data(), double(R)/10.));
            closuredet->SetNameTitle(Form("closuredet%s_R%02d", observable.data(), R), Form("Det. level input spectrum for closure test for %s for R=%.1f", observable.data(), double(R)/10.));

            cobjects.Add(closureresponsematrix);
            cobjects.Add(closureeffkine);
            cobjects.Add(closuretruth);
            cobjects.Add(closuredet);

            // Extract jet finding purity, efficiency and det / part spectrum before matching (for efficiency / purity correction)
            auto sparsePurityCT = static_cast<THnSparse *>(histlist->FindObject(Form("h%sJetFindingPurityClosureResp", observable.data()))),
                 sparseEfficiencyCT = static_cast<THnSparse *>(histlist->FindObject(Form("h%sJetFindingEfficiencyClosureResp", observable.data())));
            if(sparsePurityCT) {
                auto jetfindingpurityClosure = extractEfficiencyPurity(sparsePurityCT, detptbinning, obsmax);
                jetfindingpurityClosure->SetDirectory(nullptr);
                jetfindingpurityClosure->SetNameTitle(Form("JetFindingPurityClosure_%s_R%02d", observable.data(), R), Form("Jet finding purity for %s for R = %.1f (for closure test)", observable.data(), double(R)/10.));
                cobjects.Add(jetfindingpurityClosure);
            }
            if(sparseEfficiencyCT) {
                auto jetfindingeffClosure = extractEfficiencyPurity(sparseEfficiencyCT, partptbinning, obsmax); 
                jetfindingeffClosure->SetDirectory(nullptr);
                jetfindingeffClosure->SetNameTitle(Form("JetFindingEffClosure_%s_R%02d", observable.data(), R), Form("Jet finding efficiency for %s for R = %.1f (for closure test)", observable.data(), double(R)/10.));
                cobjects.Add(jetfindingeffClosure);
            }
            auto histClosureTruthAll = makeClosureTruthAll(histlist, observable.data(), partptbinning, obsmax),
                 histClosureDetAll = makeClosureDetAll(histlist, observable.data(), detptbinning, obsmax);
            if(histClosureTruthAll) {
               histClosureTruthAll->SetNameTitle(Form("closuretruthAll%s_R%02d", observable.data(), R), Form("Truth spectrum (closure test, before part/det matching) for observable %s for R=%.1f", observable.data(), double(R)/10.));
               cobjects.Add(histClosureTruthAll);
            }
            if(histClosureDetAll) {
                histClosureDetAll->SetNameTitle(Form("closuredetAll%s_R%02d", observable.data(), R), Form("Det. level input spectrum for closure test (before part/det matching) for %s for R=%.1f", observable.data(), double(R)/10.));
                cobjects.Add(histClosureDetAll);
            }

            // Make non-fully efficient closure truth from THnSparse
            auto noclosureresponsedata = dynamic_cast<THnSparse *>(histlist->FindObject(Form("h%sResponseClosureNoRespSparse", observable.data())));
            if(noclosureresponsedata){
                std::cout << "Building non-fully efficient truth (" << detptbinning.front() << " ... " << detptbinning.back() << ")" << std::endl;
                noclosureresponsedata->GetAxis(1)->SetRangeUser(detptbinning.front() + 1e-5, detptbinning.back() - 1e-5);
                auto h2truecutfine = noclosureresponsedata->Projection(3,2, "e");
                auto h2truecut = makeRebinnedPt(h2truecutfine, partptbinning, obsmax);
                h2truecut->SetDirectory(nullptr);
                h2truecut->SetNameTitle(Form("closuretruthcut%s_R%02d", observable.data(), R), Form("Truth spectrum (closure test) for observable %s for R=%.1f, cut det. pt", observable.data(), double(R)/10.));
                cobjects.Add(h2truecut);
            }

            std::cout << observable << " done ..." << std::endl;
        }
        objects[R] = outputobjects;
        closureobjects[R] = cobjects;
    }

    std::unique_ptr<TFile> writer(TFile::Open("responsematrix.root", "RECREATE"));
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string outputdir = Form("Response_R%02d", R);
        writer->mkdir(outputdir.data());
        writer->cd(outputdir.data());
        auto workdir =  static_cast<TDirectory *>(gDirectory);
        workdir->mkdir("response");
        workdir->cd("response");
        auto histos = objects[R];
        for(auto o : histos) o->Write();
        workdir->mkdir("closuretest");
        workdir->cd("closuretest");
        auto closurehistos = closureobjects[R];
        for(auto o : closurehistos) o->Write();
    }
    
}