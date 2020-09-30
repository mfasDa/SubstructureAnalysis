#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/roounfold.C"
#include "../../helpers/math.C"
#include "../binnings/binningPt2D.C"

RooUnfoldResponse *makeResponse(THnSparse *responsedata, std::vector<double> &detptbinning, std::vector<double> &partptbinning, const TH2 *priorweight, double obsmax = -1) {
    std::vector<double> detobsbinning, partobsbinning;
    auto detaxis = responsedata->GetAxis(0), partaxis = responsedata->GetAxis(2);
    detobsbinning.emplace_back(detaxis->GetBinLowEdge(1));
    partobsbinning.emplace_back(partaxis->GetBinLowEdge(1));
    for(auto bin : ROOT::TSeqI(0, partaxis->GetNbins())) {
        auto step = partaxis->GetBinUpEdge(bin+1);
        if(step > obsmax) continue;
        partobsbinning.emplace_back(step);
    } 
    for(auto bin : ROOT::TSeqI(0, detaxis->GetNbins())) {
        auto step = detaxis->GetBinUpEdge(bin+1);
        if(step > obsmax) continue;
        detobsbinning.emplace_back(step);
    } 
    TH2F dettemplate("dettemplate", "det template", detobsbinning.size() - 1, detobsbinning.data(), detptbinning.size() - 1, detptbinning.data()),
         parttemplate("parttemplate", "part template", partobsbinning.size() - 1, partobsbinning.data(), partptbinning.size() - 1, partptbinning.data());

    auto &ptpartmin = partptbinning.front(),
         &ptpartmax = partptbinning.back(),
         &ptdetmin = detptbinning.front(),
         &ptdetmax = detptbinning.back();

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
            std::cout << "Point (" << ptpart << " part - " << ptdet << " det) outside range" << std::endl;
            continue;
        }
        // include priorweight
        int binpriorobs = priorweight->GetXaxis()->FindBin(zgpart),
            binpriorpt = priorweight->GetYaxis()->FindBin(ptpart);
        double weightprior = priorweight->GetBinContent(binpriorobs, binpriorpt);
        resultresponse->Fill(zgdet, ptdet, zgpart, ptpart, weight * weightprior);
    }
    return resultresponse;
}

TH2 *makeRebinnedPt(const TH2 *inputspectrum, std::vector<double> partptbinning) {
    auto partaxis = inputspectrum->GetXaxis();
    std::vector<double> partobsbinning;
    partobsbinning.emplace_back(partaxis->GetBinLowEdge(1)); 
    for(auto bin : ROOT::TSeqI(0, partaxis->GetNbins())) partobsbinning.emplace_back(partaxis->GetBinUpEdge(bin+1));
    
    return makeRebinned2D(inputspectrum, partobsbinning, partptbinning);
}

TH2 *rebinDist(TH2 *hist, std::vector<double> ptbinning) {
    std::vector<double> obsbinning;
    obsbinning.push_back(hist->GetXaxis()->GetBinLowEdge(1));
    for(auto ib : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())){
        obsbinning.push_back(hist->GetXaxis()->GetBinUpEdge(ib+1));
    }
    auto rebinned = makeRebinned2D(hist, obsbinning, ptbinning);
    rebinned->SetDirectory(nullptr);
    return rebinned;
}

std::map<std::string, TH2 *> readPriorDist(TFile &priorsreader, int R, std::vector<double> ptbinning) {
    std::map<std::string, TH2 *> result;
    std::vector<std::string> observables = {"Zg", "Rg", "Nsd", "Thetag"};
    TString rstring(Form("R%02d", R));
    for(auto obs : observables) {
        priorsreader.cd(obs.data());
        TH2 *hist(nullptr);
        for(auto h : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
            TString keyname(h->GetName());
            if(keyname.Contains(rstring)) {
                hist = h->ReadObject<TH2>();
                break;
            }
        }
        if(hist) {
            std::cout << "Found histogram with name " << hist->GetName() << " for observable " << obs << " and " << rstring << std::endl;
            auto rebinned = rebinDist(hist, ptbinning);
            rebinned->SetName(Form("RebinnedPrior%s%s", obs.data(), rstring.Data()));
            result[obs] = rebinned;
        } else {
            std::cout << "Not found histogram for " << obs << " and " << rstring << std::endl; 
        }
    }
    return result;
}

TH2 *extractKinematicEfficiency(THnSparse *responsedata, std::vector<double> & partptbinning, double detptmin, double detptmax, double obsmax) {
    std::cout << "Extracting kinematic efficiency for det. pt range from " << detptmin << " GeV/c to " << detptmax << " GeV/c" << std::endl;
    std::vector<double> partobsbinning;
    auto partaxis = responsedata->GetAxis(2);
    partobsbinning.emplace_back(partaxis->GetBinLowEdge(1)); 
    for(auto bin : ROOT::TSeqI(0, partaxis->GetNbins())) {
        auto step = partaxis->GetBinUpEdge(bin+1);
        if(step > obsmax) continue;
        partobsbinning.emplace_back(step);
    } 
    
    std::unique_ptr<TH2> truefull(responsedata->Projection(3, 2));
    truefull->SetName("truefull");
    responsedata->GetAxis(1)->SetRangeUser(detptmin, detptmax);
    std::unique_ptr<TH2> truncated(responsedata->Projection(3, 2));
    truncated->SetName("truncated");
    responsedata->GetAxis(1)->SetRange(0, responsedata->GetAxis(1)->GetNbins()+1);

    TH2 *result = new TH2D("effKine", "Kinematic efficiency", partobsbinning.size()-1, partobsbinning.data(), partptbinning.size()-1, partptbinning.data());
    result->SetDirectory(nullptr);
    for(auto iptbin : ROOT::TSeqI(0, result->GetYaxis()->GetNbins())){
        double ptmin = result->GetYaxis()->GetBinLowEdge(iptbin+1),
               ptmax = result->GetYaxis()->GetBinUpEdge(iptbin+1);
        std::cout << "Doing bin " << iptbin << "(" << ptmin << " ... " << ptmax << ")" << std::endl;
        std::unique_ptr<TH1> slicefull(truefull->ProjectionX("slicefull", truefull->GetYaxis()->FindBin(ptmin), truefull->GetYaxis()->FindBin(ptmax))),
                             slicetruncated(truncated->ProjectionX("slicetruncated", truncated->GetYaxis()->FindBin(ptmin), truncated->GetYaxis()->FindBin(ptmax))); 
        slicetruncated->Divide(slicetruncated.get(), slicefull.get(), 1., 1., "b");
        for(auto iobsbin : ROOT::TSeqI(0, slicetruncated->GetXaxis()->GetNbins())){
            result->SetBinContent(iobsbin+1, iptbin+1, slicetruncated->GetBinContent(iobsbin+1));
            result->SetBinError(iobsbin+1, iptbin+1, slicetruncated->GetBinError(iobsbin+1));
        }
    }
    std::cout << "Finish projection " << std::endl;

    return result;
}

void buildResponseMatrixFromTHnSparsePriors(const char *filename = "AnalysisResults.root", const char *priorsfile = "jetspectrum_merged.root", const char *detbinningvar = "default") {
    std::vector<double> detptbinning = getDetPtBinning(detbinningvar),
                        partptbinning = {0, 10, 15, 20, 30, 40, 50, 60, 80, 100, 120, 140, 160, 180, 200, 240, 500};
    if(!detptbinning.size()) {
        std::cout << "Cannot create detector level pt binning for variation " << detbinningvar << std::endl;
        return;
    }

    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ")),
                           priorsreader(TFile::Open(priorsfile, "READ"));

    std::vector<std::string> observables = {"Zg", "Rg", "Nsd", "Thetag"};
    std::map<int, TObjArray> objects;
    std::map<int, TObjArray> closureobjects;
    std::map<int, TObjArray> priorsobjects;

    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring = Form("R%02d", R);
        auto priorhists = readPriorDist(*priorsreader, R, partptbinning);
        std::cout << "Doing jet radius R=" << (double(R)/10.) << std::endl;
        reader->cd(Form("SoftDropResponse_FullJets_R%02d_INT7", R));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        TObjArray outputobjects, cobjects, pobjects;
        for(auto observable : observables) {
            double obsmax = DBL_MAX;
            std::cout << "Next observable " << observable << std::endl;
            THnSparse* responsedata = dynamic_cast<THnSparse *>(histlist->FindObject(Form("h%sResponseSparse", observable.data())));           
            std::cout << "Extracting response matrix for observable " << observable << std::endl;
            if(!responsedata) {
                std::cerr << "Did not find response matrix sparse for observable " << observable << std::endl;
                histlist->ls();
            }

            // calculate prior weights
            // defined as variation / original
            std::unique_ptr<TH2> truefull(responsedata->Projection(3, 2));
            truefull->SetName("truefull");
            auto rebinnedOrig = rebinDist(truefull.get(), partptbinning);
            rebinnedOrig->SetName(Form("RebinnedOrig%s%s", observable.data(), rstring.data()));
            auto priorweight = static_cast<TH2 *>(priorhists.find(observable)->second->Clone(Form("Priorweights%s%s", observable.data(), rstring.data())));
            priorweight->SetDirectory(nullptr);
            priorweight->Divide(rebinnedOrig);
            pobjects.Add(rebinnedOrig);
            pobjects.Add(priorhists.find(observable)->second);
            pobjects.Add(priorweight);
            
            auto responsematrix = makeResponse(responsedata, detptbinning, partptbinning, priorweight, obsmax);
            responsematrix->SetNameTitle(Form("Responsematrix%s_R%02d", observable.data(), R), Form("Response matrix for %s for R=%.1f", observable.data(), double(R)/10.));
            std::cout << "Extracting kinematic efficiency for observable " << observable << std::endl;
            auto effKine = extractKinematicEfficiency(responsedata, partptbinning, *detptbinning.begin(), *detptbinning.rbegin(), obsmax);
            effKine->SetNameTitle(Form("EffKine%s_R%02d", observable.data(), R), Form("Kinematic efficiency for %s for R=%.1f", observable.data(), double(R)/10.));
            outputobjects.Add(responsematrix);
            outputobjects.Add(effKine);

            // Creating objects for the closure test
            auto closureresponsedata = dynamic_cast<THnSparse *>(histlist->FindObject(Form("h%sResponseClosureSparse", observable.data())));
            std::cout << "Extracting closure response matrix for observable " << observable << std::endl;
            if(!closureresponsedata) {
                std::cerr << "Did not find closure response matrix sparse for observable " << observable << std::endl;
                histlist->ls();
            }
            auto closureresponsematrix = makeResponse(closureresponsedata, detptbinning, partptbinning, priorweight, obsmax);
            closureresponsematrix->SetNameTitle(Form("ResponsematrixClosure%s_R%02d", observable.data(), R), Form("Response matrix for closure test for %s for R=%.1f", observable.data(), double(R)/10.));
            std::cout << "Extracting closure kinematic efficiency for observable " << observable << std::endl;
            auto closureeffkine = extractKinematicEfficiency(closureresponsedata, partptbinning, *detptbinning.begin(), *detptbinning.rbegin(), obsmax);
            closureeffkine->SetNameTitle(Form("EffKineClosure%s_R%02d", observable.data(), R), Form("Kinematic efficiency for %s for R=%.1f", observable.data(), double(R)/10.));
            std::cout << "Extracting det. level spectrum and true spectrum (opposite sample)" << std::endl;
            auto closuretruth = makeRebinnedPt(static_cast<TH2 *>(histlist->FindObject(Form("h%sPartLevelClosureNoRespFine", observable.data()))), partptbinning),
                 closuredet = makeRebinnedPt(static_cast<TH2 *>(histlist->FindObject(Form("h%sDetLevelClosureNoRespFine", observable.data()))), detptbinning);
            closuretruth->SetDirectory(nullptr);
            closuredet->SetDirectory(nullptr);
            closuretruth->SetNameTitle(Form("closuretruth%s_R%02d", observable.data(), R), Form("Truth spectrum (closure test) for observable %s for R=%.1f", observable.data(), double(R)/10.));
            closuredet->SetNameTitle(Form("closuredet%s_R%02d", observable.data(), R), Form("Det. level input spectrum for closure test for %s for R=%.1f", observable.data(), double(R)/10.));

            cobjects.Add(closureresponsematrix);
            cobjects.Add(closureeffkine);
            cobjects.Add(closuretruth);
            cobjects.Add(closuredet);

            // Make non-fully efficient closure truth from THnSparse
            auto noclosureresponsedata = dynamic_cast<THnSparse *>(histlist->FindObject(Form("h%sResponseClosureNoRespSparse", observable.data())));
            if(noclosureresponsedata){
                std::cout << "Building non-fully efficient truth (" << detptbinning.front() << " ... " << detptbinning.back() << ")" << std::endl;
                noclosureresponsedata->GetAxis(1)->SetRangeUser(detptbinning.front() + 1e-5, detptbinning.back() - 1e-5);
                auto h2truecutfine = noclosureresponsedata->Projection(3,2);
                auto h2truecut = makeRebinnedPt(h2truecutfine, partptbinning);
                h2truecut->SetDirectory(nullptr);
                h2truecut->SetNameTitle(Form("closuretruthcut%s_R%02d", observable.data(), R), Form("Truth spectrum (closure test) for observable %s for R=%.1f, cut det. pt", observable.data(), double(R)/10.));
                cobjects.Add(h2truecut);
            }

            std::cout << observable << " done ..." << std::endl;
        }
        objects[R] = outputobjects;
        closureobjects[R] = cobjects;
        priorsobjects[R] = pobjects;
    }

    std::unique_ptr<TFile> writer(TFile::Open("responsematrix.root", "RECREATE"));
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string outputdir = Form("Response_R%02d", R);
        writer->mkdir(outputdir.data());
        writer->cd(outputdir.data());
        auto workdir =  gDirectory;
        workdir->mkdir("response");
        workdir->cd("response");
        auto histos = objects[R];
        for(auto o : histos) o->Write();
        workdir->mkdir("closuretest");
        workdir->cd("closuretest");
        auto closurehistos = closureobjects[R];
        for(auto o : closurehistos) o->Write();
        auto priorshists = priorsobjects[R];
        for(auto o : priorshists) o->Write();
    }
    
}