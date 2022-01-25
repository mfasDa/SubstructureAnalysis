#include "../../meta/stl.C"
#include "../../meta/root.C"

struct Data {
    double fEffVtx;
    double fCENTNORDCorrection;
    double fEventsANY;
    double fWeightedEventsAny;
};

double buildVertexFindingEfficiency(TH1 *normalizationhist) {
    return normalizationhist->GetBinContent(normalizationhist->GetXaxis()->FindBin("Vertex reconstruction and quality")) / normalizationhist->GetBinContent(normalizationhist->GetXaxis()->FindBin("Event selection"));
}

double buildCENTNOTRDCorrection(TH1 *clustercounter) {
    return clustercounter->GetBinContent(3)/clustercounter->GetBinContent(2);
}

Data read(TFile &reader, const std::string_view trigger, const std::string_view sysvar) {
    reader.cd(Form("JetSpectrum_FullJets_R02_%s_%s", trigger.data(), sysvar.data()));
    auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();

    auto histoweighted = static_cast<TH1 *>(histos->FindObject("hClusterCounter")),
         histounweighted = static_cast<TH1 *>(histos->FindObject("hClusterCounterAbs"));

    Data result;
    result.fEffVtx = buildVertexFindingEfficiency(static_cast<TH1 *>(histos->FindObject("fNormalisationHist"))); // take the one from the AliAnalysisTaskEmcal directly
    result.fCENTNORDCorrection = buildCENTNOTRDCorrection(histounweighted);
    result.fEventsANY = histounweighted->GetBinContent(1);
    result.fWeightedEventsAny = histoweighted->GetBinContent(1);
    return result;
}


void extractLuminositiesFromRawOutput(int year, const std::string_view rootfile = "AnalysisResults.root", const std::string_view sysvar = "tc200"){
    std::unique_ptr<TFile> reader(TFile::Open(rootfile.data(), "READ"));

    std::map<int, double> crosssections = {{2017, 58.10}, {2018, 57.52}};

    double xsec = -1.;
    auto xsfound = crosssections.find(year);
    if(xsfound != crosssections.end()) {
        xsec = xsfound->second; 
    } else {
        std::cout << "No reference cross section found for year " << year << std::endl;
        return;
    }
    std::cout << "Using reference cross section: " << xsec << "mb-1" << std::endl;
    // Convert cross section to picobarn
    xsec *= 1e9;

    std::array<std::string, 3> triggers = {{"INT7", "EJ2", "EJ1"}};
    std::map<std::string, Data> triggerdata;
    for(const auto &trg : triggers) triggerdata[trg] = read(*reader, trg, sysvar);

    double effVertex = triggerdata["INT7"].fEffVtx;
    std::cout << "Using vertex finding efficiency :" << effVertex << std::endl;

    double centnotrdcorrection = triggerdata["EJ1"].fCENTNORDCorrection;
    std::cout << "Using CENTNOTRD correction: " << centnotrdcorrection << std::endl;

    // EJ2: Calculate average downscaling
    double downscalingEJ2 = triggerdata["EJ2"].fEventsANY/triggerdata["EJ2"].fWeightedEventsAny;
    std::cout << "Average downscaling for EJ2: " << downscalingEJ2 << std::endl;

    double eventsINT7 = triggerdata["INT7"].fEventsANY,
           reflumi = triggerdata["INT7"].fWeightedEventsAny;
    std::cout << "Using reference luminosity: " << reflumi << std::endl;
    double lumiInt7 = eventsINT7 / (effVertex * xsec);
    double eventsEJ2 = triggerdata["EJ2"].fEventsANY;
    double lumiEJ2 = reflumi * downscalingEJ2 / (effVertex * xsec);
    double eventsEJ1 = triggerdata["EJ1"].fEventsANY;
    double lumiEJ1 = reflumi * centnotrdcorrection / (effVertex * xsec);
    std::cout << "INT: " << eventsINT7 << " Events, Lint = " << lumiInt7 << " pb-1" << std::endl;
    std::cout << "EJ2: " << eventsEJ2 << " Events, Lint = " << lumiEJ2 << " pb-1" << std::endl;
    std::cout << "EJ1: " << eventsEJ1 << " Events, Lint = " << lumiEJ1 << " pb-1" << std::endl;

}