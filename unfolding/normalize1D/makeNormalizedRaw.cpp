#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/math.C"
#include "../binnings/binningPt1D.C"

struct SpectrumAndNorm {
    TH1 *fSpectrum;
    double fEventCount;
    double fEventCountAbsolute;
    double fVertexFindingEfficiency;

    double getNorm() const { return fEventCount / fVertexFindingEfficiency; }
};

SpectrumAndNorm getSpectrumAndNorm(TFile &reader, double R, const std::string_view trigger, const std::string_view triggercluster, const std::string_view sysvar) {
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
    // Get absolute eventCounts
    auto heventcounts = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));
    auto eventcounts = hnorm->GetBinContent(clusterbin);
    // calculate vertex finding efficiency
    auto evhist = static_cast<TH1 *>(histos->FindObject("fNormalisationHist")); // take the one from the AliAnalysisTaskEmcal directly
    auto vertexfindingeff = evhist->GetBinContent(evhist->GetXaxis()->FindBin("Vertex reconstruction and quality")) / evhist->GetBinContent(evhist->GetXaxis()->FindBin("Event selection"));
    SpectrumAndNorm result{rawspectrum, norm, eventcounts, vertexfindingeff};
    return result;
}

TH1 *makeRebinnedSafe(const TH1 *input, const char *newname, const std::vector<double> binning) {
    std::unique_ptr<TH1> rebinhelper(histcopy(input));
    auto rebinned = rebinhelper->Rebin(binning.size()-1, newname, binning.data());
    return rebinned;
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

TH1 *makeEventCounterHistogram(const char *name, const char *title, double countEJ1, double countEJ2, double countINT7) { 
    auto eventCount = new TH1F(name, title, 3, 0., 3);
    eventCount->GetXaxis()->SetBinLabel(1, "INT7");
    eventCount->GetXaxis()->SetBinLabel(2, "EJ2");
    eventCount->GetXaxis()->SetBinLabel(3, "EJ1");
    eventCount->SetBinContent(1, countINT7);
    eventCount->SetBinContent(2, countEJ2);
    eventCount->SetBinContent(3, countEJ1);
    return eventCount;
}

TH1 *makeTriggerEfficiency(TFile &mcreader, int R, const std::string_view trigger, const std::string_view sysvar, const std::vector<double> *binning = nullptr){
    std::unique_ptr<TH1> mbref(getSpectrumAndNorm(mcreader, R, "INT7", "ANY", sysvar).fSpectrum),
                         trgspec(getSpectrumAndNorm(mcreader, R, trigger, "ANY", sysvar).fSpectrum);
    TH1 *eff = nullptr,
        *numerator = trgspec.get(),
        *denominator = mbref.get();
    std::string histname = Form("TriggerEfficiency_%s_R%02d", trigger.data(), R),
                histtitle = Form("Trigger efficiency for %s for R=%.1f", trigger.data(), double(R)/10.);
    if(binning) {
        // Make rebin on clone, to be on the safe side as the rebin function is not marked const
        std::unique_ptr<TH1> triggeredRebinned(makeRebinnedSafe(trgspec.get(),Form("%s_rebinned", trgspec->GetName()), *binning)); 
        std::unique_ptr<TH1> minbiasRebinned(makeRebinnedSafe(mbref.get(), Form("%s_rebinned", mbref->GetName()), *binning));
        eff = histcopy(triggeredRebinned.get());
        eff->Divide(triggeredRebinned.get(), minbiasRebinned.get(), 1., 1., "b");
    } else {
        eff = histcopy(trgspec.get());
        eff->Divide(trgspec.get(), mbref.get(), 1., 1., "b");
    }
    eff->SetNameTitle(histname.data(), histtitle.data());
    eff->SetDirectory(nullptr);
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

void makeNormalizedRaw(const std::string_view datafile, const std::string_view mcfile, const std::string_view sysvar = ""){
    std::stringstream outputfile;
    outputfile << "correctedSVD_poor";
    if(sysvar.length()) {
        outputfile << "_" << sysvar;
    }
    outputfile << ".root";
    std::unique_ptr<TFile> datareader(TFile::Open(datafile.data(), "READ")),
                           writer(TFile::Open(outputfile.str().data(), "RECREATE"));
    std::unique_ptr<TFile> mcreader;
    if(mcfile.length()) {
        mcreader = std::unique_ptr<TFile>(TFile::Open(mcfile.data(), "READ"));
    } 
    auto binningpart = getJetPtBinningNonLinTruePoor(),
         binningdet = getJetPtBinningNonLinSmearPoor();
    auto centnotrdcorrection = getCENTNOTRDCorrection(*datareader, sysvar);
    double crosssection = 57.8;
    for(auto R : ROOT::TSeqI(2, 7)) {
        double radius = double(R) / 10.;
        std::cout << "Doing jet radius " << radius << std::endl;
        auto mbspectrum = getSpectrumAndNorm(*datareader, radius, "INT7", "ANY", sysvar),
             ej1spectrum = getSpectrumAndNorm(*datareader, radius, "EJ1", "CENTNOTRD", sysvar),
             ej2spectrum = getSpectrumAndNorm(*datareader, radius, "EJ2", "CENT", sysvar);

        // apply CENTNOTRD correction
        ej1spectrum.fSpectrum->Scale(1./centnotrdcorrection);

        // Uncorrected rebinned Spectra (for monitoring)
        TH1 *mbrebinned = makeRebinnedSafe(mbspectrum.fSpectrum, "mbrebinned", binningdet),
            *ej1rebinnedUncorrected = makeRebinnedSafe(ej1spectrum.fSpectrum, "ej1rebinnedUncorrected", binningdet),
            *ej2rebinnedUncorrected = makeRebinnedSafe(ej1spectrum.fSpectrum, "ej2rebinnedUncorrected", binningdet);

        // Write everything to file
        writer->mkdir(Form("R%02d", int(radius*10)));
        writer->cd(Form("R%02d", int(radius*10)));
        auto basedir = gDirectory;
        basedir->mkdir("rawlevel");
        basedir->cd("rawlevel");
        mbspectrum.fSpectrum->Write();
        ej1spectrum.fSpectrum->Write();
        ej2spectrum.fSpectrum->Write();
        mbrebinned->Write();
        ej1rebinnedUncorrected->Write();
        ej2rebinnedUncorrected->Write();
        auto hnorm = new TH1F("hNorm", "Norm", 1, 0.5, 1.5);
        hnorm->SetBinContent(1, mbspectrum.getNorm());
        hnorm->Write();
        makeEventCounterHistogram("hEventCounterWeighted", "Weighted event counts", ej1spectrum.fEventCount, ej2spectrum.fEventCount, mbspectrum.fEventCount)->Write();
        makeEventCounterHistogram("hEventCounterAbs", "Absolute event counts", ej1spectrum.fEventCountAbsolute, ej2spectrum.fEventCountAbsolute, mbspectrum.fEventCountAbsolute)->Write();
        auto heffVtx = new TH1F("hVertexFindingEfficiency", "Vertex finding efficiency", 1, 0.5, 1.5);
        heffVtx->SetBinContent(1, mbspectrum.fVertexFindingEfficiency);
        heffVtx->Write();
        auto hcntcorr = new TH1F("hCENTNOTRDcorrection", "CENTNOTRD correction", 1, 0.5, 1.5);
        hcntcorr->SetBinContent(1, centnotrdcorrection);
        hcntcorr->Write();

        if(mcreader) {
            // Correct for the trigger efficiency
            // Only in mode with MC (i.e. not when running on single periods)
            auto trgeffej1 = makeTriggerEfficiency(*mcreader, radius, "EJ1", sysvar),
                 trgeffej2 = makeTriggerEfficiency(*mcreader, radius, "EJ2", sysvar),
                 rebinnedTriggerEffEJ1 = makeTriggerEfficiency(*mcreader, R, "EJ1", sysvar, &binningdet),
                 rebinnedTriggerEffEJ2 = makeTriggerEfficiency(*mcreader, R, "EJ1", sysvar, &binningdet);
                 
            ej1spectrum.fSpectrum->Divide(trgeffej1);
            ej2spectrum.fSpectrum->Divide(trgeffej2);

            // Rebin all raw level histograms
            TH1 *ej1rebinned(ej1spectrum.fSpectrum->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data())),
                *ej2rebinned(ej2spectrum.fSpectrum->Rebin(binningdet.size()-1, "ej1rebinned", binningdet.data()));
            auto hraw = makeCombinedRawSpectrum(*mbrebinned, *ej2rebinned, 50., *ej1rebinned, 100.);
            hraw->SetNameTitle(Form("hraw_R%02d", int(radius * 10.)), Form("Raw Level spectrum R=%.1f", radius));
            hraw->Scale(crosssection/mbspectrum.getNorm());

            trgeffej1->Write();
            trgeffej2->Write();
            rebinnedTriggerEffEJ1->Write();
            rebinnedTriggerEffEJ2->Write();
            ej1rebinned->Write();
            ej2rebinned->Write();
            hraw->Write();
        }
    }
}