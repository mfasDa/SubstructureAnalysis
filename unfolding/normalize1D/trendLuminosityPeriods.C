#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

#include "../../struct/DataFileHandler.cxx"
#include "../../struct/LuminosityHandler.cxx"

struct RawLuminosities{
    std::map<std::string, double> luminosity;
    std::map<std::string, double> eventcounts;
    double centnotrdcorrection;
    double downscalingEJ2;
    double downscalingINT7;
    double vertexfindingeff;
};

class TrendingHistogram {
    public:
        TrendingHistogram() = default;
        TrendingHistogram(const std::string_view name, const std::string_view title, const std::string_view ytitle, const std::set<std::string> &periods) : fRawHistogram(nullptr) {
            fRawHistogram = new TH1D(name.data(), title.data(), periods.size(), 0, periods.size());
            fRawHistogram->SetDirectory(nullptr);
            fRawHistogram->SetStats(false);
            fRawHistogram->GetYaxis()->SetTitle(ytitle.data());
            int currentbin = 0;
            for(const auto &period : periods) {
                fRawHistogram->GetXaxis()->SetBinLabel(currentbin+1, period.data());
                currentbin++;
            }
        }
        ~TrendingHistogram() = default;

        TH1 *getHistogram() const { return fRawHistogram; }

        void SetValueForPeriod(const std::string_view period, double value) {
            int binID = fRawHistogram->GetXaxis()->FindBin(period.data());
            fRawHistogram->SetBinContent(binID, value);
        }

        void Write() { fRawHistogram->Write(); }
    private:
        TH1 *fRawHistogram = nullptr;
};

std::set<std::string> findPeriods(const std::string_view inputdir, const std::string_view analysisfile = "AnalysisResults.root") {
    std::set<std::string> result;
    auto content = gSystem->GetFromPipe(Form("ls -1 %s | grep LHC", inputdir.data()));
    std::unique_ptr<TObjArray> tokens(content.Tokenize("\n"));
    for(auto dir : TRangeDynCast<TObjString>(*tokens)) {
        TString testfile = Form("%s/%s/%s", inputdir.data(), dir->String().Data(), analysisfile.data());
        if(!gSystem->AccessPathName(testfile.Data())) {
            result.insert(dir->String().Data());
        }
    } 
    return result;
}

RawLuminosities getLuminosities(const std::string_view inputfile) {
    using Trigger = LuminosityHandler::TriggerClass;
    RawLuminosities result;
    std::map<Trigger, std::string> triggernames = {{Trigger::INT7, "INT7"}, {Trigger::EJ2, "EJ2"}, {Trigger::EJ1, "EJ1"}};
    DataFileHandler datahandler(inputfile, "FullJets", "tc200");
    LuminosityHandler lumihandler(datahandler);
    for(const auto &[trg, name] : triggernames) {
        result.eventcounts[name] = lumihandler.getRawEvents(trg);
        result.luminosity[name] = lumihandler.getLuminosity(trg, true);
    }
    result.centnotrdcorrection = lumihandler.getCENTNOTRDCorrection();
    result.downscalingEJ2 = lumihandler.getEffectiveDownscaleing(Trigger::EJ2);
    result.downscalingINT7 = lumihandler.getEffectiveDownscaleing(Trigger::INT7);
    result.vertexfindingeff = lumihandler.getVertexFindingEfficiency();
    return result;
}

void trendLuminosityPeriods(const std::string_view inputdir = "", const std::string_view analysisFile = "AnalysisResults.root"){
    std::map<std::string, RawLuminosities> periodLumis;
    std::string datainputdir = gSystem->pwd();
    if(inputdir.length()) datainputdir = inputdir;
    auto periods = findPeriods(datainputdir);
    for(auto &period : periods) periodLumis[period] = getLuminosities(Form("%s/%s/%s", datainputdir.data(), period.data(), analysisFile.data()));

    std::array<std::string, 3> triggers = {{"INT7", "EJ2", "EJ1"}};
    std::map<std::string, TrendingHistogram> luminosityTrendings, eventTrendings;
    TrendingHistogram ej2downscaleTrending("EJ2downscaleTrending", "Trending of the average EJ2 downscaling", "<EJ2 downscaling>", periods),
                      int7downscaleTrending("INT7downscaleTrending", "Trending of the average INT7 downscaling", "<INT7 downscaling>", periods),
                      centnotrdtrending("CENTNOTRDTrending", "Trending of the CENTNOTRD correction", "CENTNOTD correction factor", periods),
                      vertextrending("VertexEffTrending", "Trending of the vertex finding efficiency", "#epsilon_{vtx}", periods);
    bool commontrendings = false;
    for(const auto &trg : triggers) {
        auto lumihist = TrendingHistogram(Form("Luminosity%s",trg.data()), Form("Luminosity trigger %s", trg.data()), "L_{int} (pb^{-1})", periods),
             eventhist = TrendingHistogram(Form("Luminosity%s",trg.data()), Form("Number of %s events", trg.data()), "N_{events}", periods);
        for(const auto &[period, data] : periodLumis) {
            lumihist.SetValueForPeriod(period, data.luminosity.find(trg)->second);
            eventhist.SetValueForPeriod(period, data.eventcounts.find(trg)->second);
            if(!commontrendings) {
                ej2downscaleTrending.SetValueForPeriod(period, data.downscalingEJ2);
                int7downscaleTrending.SetValueForPeriod(period, data.downscalingINT7);
                centnotrdtrending.SetValueForPeriod(period, data.centnotrdcorrection);
                vertextrending.SetValueForPeriod(period, data.vertexfindingeff);
            }
        }
        if(!commontrendings) commontrendings = true;
        luminosityTrendings[trg] = lumihist;
        eventTrendings[trg] = eventhist;
    }

    // Draw
    auto lumiplot = new ROOT6tools::TSavableCanvas("trendingLuminosity", "Luminosity trending", 1200, 600);
    lumiplot->Divide(3,1);
    int currentpad = 1;
    for(const auto &[trg, hist] : luminosityTrendings) {
        lumiplot->cd(currentpad);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        auto rawhist = hist.getHistogram();
        rawhist->SetLineColor(kBlue);
        rawhist->Draw("box");
        (new ROOT6tools::TNDCLabel(0.25, 0.8, 0.35, 0.89, trg.data()))->Draw();
        currentpad++;
    }
    lumiplot->cd();
    lumiplot->Update();
    lumiplot->SaveCanvas(lumiplot->GetName());

    auto eventplot = new ROOT6tools::TSavableCanvas("trendingEvents", "Event trending", 1200, 600);
    eventplot->Divide(3,1);
    currentpad = 1;
    for(const auto &[trg, hist] : eventTrendings) {
        eventplot->cd(currentpad);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        auto rawhist = hist.getHistogram();
        rawhist->SetLineColor(kBlue);
        rawhist->Draw("box");
        (new ROOT6tools::TNDCLabel(0.2, 0.8, 0.35, 0.89, trg.data()))->Draw();
        currentpad++;
    }
    eventplot->cd();
    eventplot->Update();
    eventplot->SaveCanvas(eventplot->GetName());

    auto generalplot = new ROOT6tools::TSavableCanvas("trendingGeneral", "General trending", 1200, 800);
    generalplot->Divide(2,2);
    generalplot->cd(1);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    auto rawhist = vertextrending.getHistogram();
    rawhist->SetLineColor(kBlue);
    rawhist->Draw("box");
    generalplot->cd(2);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    rawhist = centnotrdtrending.getHistogram();
    rawhist->SetLineColor(kBlue);
    rawhist->Draw("box");
    generalplot->cd(3);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    rawhist = ej2downscaleTrending.getHistogram();
    rawhist->SetLineColor(kBlue);
    rawhist->Draw("box");
    generalplot->cd(4);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    rawhist = int7downscaleTrending.getHistogram();
    rawhist->SetLineColor(kBlue);
    rawhist->Draw("box");
    generalplot->cd();
    generalplot->Update();
    generalplot->SaveCanvas(eventplot->GetName());

    // Save to file
    std::unique_ptr<TFile> writer(TFile::Open("LuminosityTrending.root", "RECREATE"));
    writer->cd ();
    for(const auto &trigger : triggers) {
        eventTrendings[trigger].Write();
        luminosityTrendings[trigger].Write();
    }
    vertextrending.Write();
    centnotrdtrending.Write();
    ej2downscaleTrending.Write();
}