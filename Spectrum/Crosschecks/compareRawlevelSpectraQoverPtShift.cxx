#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/math.C"
#include "../../unfolding/binnings/binningPt1D.C"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/Ratio.cxx"

std::pair<double, TH1 *> getSpectrumAndNorm(TFile &reader, double R, const std::string_view trigger, const std::string_view triggercluster, const std::string_view sysvar) {
    int clusterbin = 0;
    if(triggercluster == "ANY") clusterbin = 1;
    else if(triggercluster == "CENT") clusterbin = 2;
    else if(triggercluster == "CENTNOTRD") clusterbin = 3;
    std::stringstream dirnamebuilder, histnamebuilder;
    dirnamebuilder << "JetSpectrum_FullJets_R" << std::setw(2) << std::setfill('0') << int(R*10.) << "_" << trigger;
    histnamebuilder << "RawJetSpectrum_FullJets_R" <<  std::setw(2) << std::setfill('0') << int(R*10.) << "_" << trigger << "_" << triggercluster;
    if(sysvar.length()) {
        dirnamebuilder << "_" << sysvar;
        histnamebuilder << "_" << sysvar;
    }
    reader.cd(dirnamebuilder.str().data());
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hspecandclass = static_cast<TH2 *>(histos->FindObject("hJetSpectrum"));
    auto rawspectrum = hspecandclass->ProjectionY(histnamebuilder.str().data(), clusterbin, clusterbin);
    rawspectrum->SetDirectory(nullptr);

    // calculate norm
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    auto norm = hnorm->GetBinContent(clusterbin);
    // calculate bin0 correction
    auto evhist = static_cast<TH1 *>(histos->FindObject("fNormalisationHist")); // take the one from the AliAnalysisTaskEmcal directly
    auto bin0correction = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality"));
    norm *= bin0correction;

    return {norm, rawspectrum};
}

double getCENTNOTRDCorrection(TFile &reader, const std::string_view sysvar){
    std::stringstream dirnamebuilder;
    dirnamebuilder << "JetSpectrum_FullJets_R02_EJ1";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar; 
    reader.cd(dirnamebuilder.str().data());
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hnorm = static_cast<TH1 *>(histos->FindObject("hClusterCounter"));
    return hnorm->GetBinContent(3)/hnorm->GetBinContent(2);
}

TH1 *makeTriggerEfficiency(TFile &mcreader, double R, const std::string_view trigger, const std::string_view sysvar){
    std::unique_ptr<TH1> mbref(getSpectrumAndNorm(mcreader, R, "INT7", "ANY", sysvar).second),
                         trgspec(getSpectrumAndNorm(mcreader, R, trigger, "ANY", sysvar).second);
    auto eff = histcopy(trgspec.get());
    eff->SetNameTitle(Form("TriggerEfficiency_%s_R%02d", trigger.data(), int(R * 10.)), Form("Trigger efficiency for %s for R=%.1f", trigger.data(), R));
    eff->SetDirectory(nullptr);
    eff->Divide(trgspec.get(), mbref.get(), 1., 1., "b");
    return eff;
}

TH1 *makeCombinedRawSpectrum(const TH1 &mb, const TH1 &ej2, double ej2swap, const TH1 &ej1, double ej1swap){
    auto combined = histcopy(&mb);
    combined->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(combined->GetXaxis()->FindBin(ej2swap), combined->GetXaxis()->FindBin(ej1swap))) {
        std::cout << "[" << combined->GetXaxis()->GetBinLowEdge(b) << " - " << combined->GetXaxis()->GetBinUpEdge(b) << "] Using EJ2" << std::endl;
        combined->SetBinContent(b, ej2.GetBinContent(b));
        combined->SetBinError(b, ej2.GetBinError(b));
    }
    for(auto b : ROOT::TSeqI(combined->GetXaxis()->FindBin(ej1swap), combined->GetXaxis()->GetNbins()+1)) {
        std::cout << "[" << combined->GetXaxis()->GetBinLowEdge(b) << " - " << combined->GetXaxis()->GetBinUpEdge(b) << "] Using EJ1" << std::endl;
        combined->SetBinContent(b, ej1.GetBinContent(b));
        combined->SetBinError(b, ej1.GetBinError(b));
    }
    return combined;
}

std::map<int, TH1 *> getRawSpectra(const std::string_view datafile, const std::string_view mcfile, const std::string_view sysvar) {
    std::map<int, TH1 *> result;
    std::unique_ptr<TFile> datareader(TFile::Open(datafile.data(), "READ")),
                           mcreader(TFile::Open(mcfile.data(), "READ"));
    auto binningpart = getJetPtBinningNonLinTruePoor(),
         binningdet = getJetPtBinningNonLinSmearPoor();
    auto centnotrdcorrection = getCENTNOTRDCorrection(*datareader, sysvar);
    double crosssection = 57.8;
    for(double radius = 0.2; radius <= 0.6; radius += 0.1) {
        std::cout << "Doing jet radius " << radius << std::endl;
        auto mbspectrum = getSpectrumAndNorm(*datareader, radius, "INT7", "ANY", sysvar),
             ej1spectrum = getSpectrumAndNorm(*datareader, radius, "EJ1", "CENTNOTRD", (TMath::Abs(radius-0.4) < 1e-12 && datafile.find("ref") != std::string::npos) ? "5" : sysvar),
             ej2spectrum = getSpectrumAndNorm(*datareader, radius, "EJ2", "CENT", sysvar);
        auto trgeffej1 = makeTriggerEfficiency(*mcreader, radius, "EJ1", "t200"),
             trgeffej2 = makeTriggerEfficiency(*mcreader, radius, "EJ2", "t200");

        // apply CENTNOTRD correction
        ej1spectrum.second->Scale(1./centnotrdcorrection);

        // Correct for the trigger efficiency
        ej1spectrum.second->Divide(trgeffej1);
        ej2spectrum.second->Divide(trgeffej2);

        // Rebin all raw level histograms
        std::unique_ptr<TH1> mbrebinned(mbspectrum.second->Rebin(binningdet.size()-1, "mbrebinned", binningdet.data())),
                             ej1rebinned(ej1spectrum.second->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data())),
                             ej2rebinned(ej2spectrum.second->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data()));
        auto hraw = makeCombinedRawSpectrum(*mbrebinned, *ej2rebinned, 50., *ej1rebinned, 100.);
        hraw->SetNameTitle(Form("hraw_R%02d", int(radius * 10.)), Form("Raw Level spectrum R=%.1f", radius));
        hraw->Scale(crosssection/mbspectrum.first);
        hraw->SetDirectory(nullptr);
        result[int(radius*10.)] = hraw;
    }
    return result;
}

void compareRawlevelSpectraQoverPtShift(const std::string_view fileshift, const std::string_view fileref, const std::string_view mcfile){
    auto rawspectrashift = getRawSpectra(fileshift, mcfile, "tc200"),
         rawspectraref = getRawSpectra(fileref, mcfile, "t200");
    
    auto plot = new ROOT6tools::TSavableCanvas("comparisonQoverPtShift", "Comparison Q/pt shift", 1500, 700);
    plot->Divide(5,2);

    Style refstyle{kBlue, 24}, shiftstyle{kRed, 25}, ratiostyle{kBlack, 20};

    int icol = 0;
    for(auto r : ROOT::TSeqI(2, 7)){
        plot->cd(icol+1);
        GraphicsPad specpad(gPad);
        specpad.Logy();
        specpad.Margins(0.15, 0.05, -1, 0.05);
        specpad.Frame(Form("specframeR%02d", r), "p_{t,det} (GeV/c)", "d#sigma/dp_{t}d#eta (mb/(GeV/c))", 0., 250., 1e-10, 1.);
        specpad.Label(0.2, 0.15, 0.4, 0.22, Form("R=%.1f", double(r)/10.));
        if(icol == 0) specpad.Legend(0.4, 0.65, 0.94, 0.94);

        auto specref = rawspectraref.find(r)->second,
             specshift = rawspectrashift.find(r)->second;
        specref->Scale(1., "width");
        specshift->Scale(1., "width");
        specpad.Draw<TH1>(specref, refstyle, "Default");
        specpad.Draw<TH1>(specshift, shiftstyle, "Q/p_{t}-shift");
        
        plot->cd(icol+6);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.15, 0.05, -1, 0.05);
        ratiopad.Frame(Form("ratioframeR%02d", r), "p_{t,det} (GeV/c)", "Shifted/Default", 0., 250., 0.5, 1.5);
        auto shiftratio = new Ratio(specshift, specref);
        ratiopad.Draw<Ratio>(shiftratio, ratiostyle);  
        TF1 *fit = new TF1(Form("FitR%02d", r), "pol1", 0, 300);
        shiftratio->Fit(fit, "", "", 10, 240);
        ratiopad.Label(0.15, 0.3, 0.9, 0.37, Form("offset: %f #pm %f", fit->GetParameter(0), fit->GetParError(0)));
        ratiopad.Label(0.15, 0.2, 0.9, 0.27, Form("slope: %f #pm %f", fit->GetParameter(1), fit->GetParError(1)));
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}