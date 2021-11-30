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

TH1 *makeEventCounterHistogram(const char *name, const char *title, double countINT7) { 
    auto eventCount = new TH1F(name, title, 1, 0.5, 1.5);
    eventCount->GetXaxis()->SetBinLabel(1, "INT7");
    eventCount->SetBinContent(1, countINT7);
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


void makeNormalizedRawCharged(const std::string_view datafile, const std::string_view mcfile, const std::string_view sysvar = ""){
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
    auto binningpart = getJetPtBinningNonLinTrueCharged(),
         binningdet = getJetPtBinningNonLinSmearCharged();
    double crosssection = 57.8;
    for(auto R : ROOT::TSeqI(2, 7)) {
        double radius = double(R) / 10.;
        std::cout << "Doing jet radius " << radius << std::endl;
        auto mbspectrum = getSpectrumAndNorm(*datareader, radius, "INT7", "ANY", sysvar);

        // Uncorrected rebinned Spectra (for monitoring)
        TH1 *mbrebinned = makeRebinnedSafe(mbspectrum.fSpectrum, "mbrebinned", binningdet);

        // Write everything to file
        writer->mkdir(Form("R%02d", int(radius*10)));
        writer->cd(Form("R%02d", int(radius*10)));
        auto basedir = gDirectory;
        basedir->mkdir("rawlevel");
        basedir->cd("rawlevel");
        mbspectrum.fSpectrum->Write();
        mbrebinned->Write();
        auto hnorm = new TH1F("hNorm", "Norm", 1, 0.5, 1.5);
        hnorm->SetBinContent(1, mbspectrum.getNorm());
        hnorm->Write();
        makeEventCounterHistogram("hEventCounterWeighted", "Weighted event counts", mbspectrum.fEventCount)->Write();
        makeEventCounterHistogram("hEventCounterAbs", "Absolute event counts", mbspectrum.fEventCountAbsolute)->Write();
        auto heffVtx = new TH1F("hVertexFindingEfficiency", "Vertex finding efficiency", 1, 0.5, 1.5);
        heffVtx->SetBinContent(1, mbspectrum.fVertexFindingEfficiency);
        heffVtx->Write();
    }
}