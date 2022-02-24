#ifndef __CLING__
#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#endif

void fixYearLabels(TH1 *hist) {
    for(auto ib : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())) {
        auto year = static_cast<int>(hist->GetXaxis()->GetBinCenter(ib+1));
        hist->GetXaxis()->SetBinLabel(ib+1, Form("%d", year));
    }
}

void drawIntegratedLuminosities(const std::string_view filename = "correctedSVD_poor_tc200.root"){
    std::map<int, TH1 *> lumiHistsForYears, eventCounterHistsForYears, downscalingHistsForYears, uncertaintyHistsForYears;
    std::map<std::string, TH1 *> lumiHistsForTriggers, eventCounterHistsForTriggers, downscalingHistsForTriggers, uncertaintyHistsForTriggers;
    TH1 *hLuminosityAllYears = nullptr;
    const std::string token_luminosity = "hLuminosities",
                      token_eventcounter = "hEventCounterAbs",
                      token_downscaling = "hObservedDownscaling",
                      token_uncertainty = "hLuminosityUncertainties";
    {
        std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
        reader->cd("R02/rawlevel");
        hLuminosityAllYears = gDirectory->Get<TH1>("hLuminositiesAllYears");
        hLuminosityAllYears->SetDirectory(nullptr);
        auto rawdir = static_cast<TDirectory *>(gDirectory);
        rawdir->cd("Luminosities");
        for(auto content : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
            const std::string_view objname = content->GetName();
            if(objname.find(token_luminosity) == std::string::npos)
                continue;
            std::cout << "Reading " << objname << std::endl;
            auto hist = content->ReadObject<TH1>();
            hist->SetDirectory(nullptr);
            if(objname.find(token_luminosity) != std::string::npos) {
                auto tag = objname.substr(token_luminosity.length());
                if(std::all_of(tag.begin(), tag.end(), ::isdigit)) {
                    auto year = std::atoi(tag.data());
                    std::cout << "Found year " << year << std::endl;
                    lumiHistsForYears[year] = hist;
                } else {
                    std::string_view triggerclass = tag;
                    std::cout << "Found trigger " << triggerclass << std::endl;
                    lumiHistsForTriggers[std::string(triggerclass)] = hist;
                }
            }
        }
        rawdir->cd("EffectiveDownscaling");
        for(auto content : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
            const std::string_view objname = content->GetName();
            if(objname.find(token_downscaling) == std::string::npos)
                continue;
            std::cout << "Reading " << objname << std::endl;
            auto hist = content->ReadObject<TH1>();
            hist->SetDirectory(nullptr);
            if(objname.find(token_downscaling) != std::string::npos) {
                auto tag = objname.substr(token_downscaling.length());
                if(std::all_of(tag.begin(), tag.end(), ::isdigit)) {
                    auto year = std::atoi(tag.data());
                    std::cout << "Found year " << year << std::endl;
                    downscalingHistsForYears[year] = hist;
                } else {
                    std::string_view triggerclass = tag;
                    std::cout << "Found trigger " << triggerclass << std::endl;
                    downscalingHistsForTriggers[std::string(triggerclass)] = hist;
                }
            }
        }
        rawdir->cd("LuminosityUncertainty");
        for(auto content : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
            const std::string_view objname = content->GetName();
            if(objname.find(token_uncertainty) == std::string::npos)
                continue;
            std::cout << "Reading " << objname << std::endl;
            auto hist = content->ReadObject<TH1>();
            hist->SetDirectory(nullptr);
            if(objname.find(token_uncertainty) != std::string::npos) {
                auto tag = objname.substr(token_uncertainty.length());
                if(std::all_of(tag.begin(), tag.end(), ::isdigit)) {
                    auto year = std::atoi(tag.data());
                    std::cout << "Found year " << year << std::endl;
                    uncertaintyHistsForYears[year] = hist;
                } else {
                    std::string_view triggerclass = tag;
                    std::cout << "Found trigger " << triggerclass << std::endl;
                    uncertaintyHistsForTriggers[std::string(triggerclass)] = hist;
                }
            }
        }
        rawdir->cd("EventCounters");
        for(auto content : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
            const std::string_view objname = content->GetName();
            if(objname.find(token_eventcounter) == std::string::npos)
                continue;
            std::cout << "Reading " << objname << std::endl;
            auto hist = content->ReadObject<TH1>();
            hist->SetDirectory(nullptr);
            if(objname.find(token_eventcounter) != std::string::npos) {
                auto tag = objname.substr(token_eventcounter.length());
                if(std::all_of(tag.begin(), tag.end(), ::isdigit)) {
                    auto year = std::atoi(tag.data());
                    std::cout << "Found year " << year << std::endl;
                    eventCounterHistsForYears[year] = hist;
                } else {
                    auto triggerclass = tag.substr(0, tag.find("Trigger"));
                    std::cout << "Found trigger " << triggerclass << std::endl;
                    eventCounterHistsForTriggers[std::string(triggerclass)] = hist;
                } 
            }
        }
    }
    std::array<std::string, 3> triggerorder = {{"INT7", "EJ2", "EJ1"}};
    int currentpad = 1;

    auto plotEventCounterYears = new ROOT6tools::TSavableCanvas("eventCounterYears", "Event counter for different years", 400 * eventCounterHistsForYears.size(), 400);
    plotEventCounterYears->Divide(eventCounterHistsForYears.size(), 1);
    for(auto &[year, hist] : eventCounterHistsForYears) {
        plotEventCounterYears->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Event counts %d", year));
        hist->Draw();
        currentpad++;
    } 
    plotEventCounterYears->cd();
    plotEventCounterYears->SaveCanvas(plotEventCounterYears->GetName());

    auto plotEventCounterTriggers = new ROOT6tools::TSavableCanvas("eventCounterTriggers", "Event counter for different trigger classes", 400 * eventCounterHistsForTriggers.size(), 400);
    plotEventCounterTriggers->Divide(eventCounterHistsForTriggers.size(), 1);
    currentpad = 1;
    for(const auto & trigger : triggerorder) {
        auto hist = eventCounterHistsForTriggers[trigger];
        plotEventCounterTriggers->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Event counts %s", trigger.data()));
        fixYearLabels(hist);
        hist->Draw();
        currentpad++;
    } 
    plotEventCounterTriggers->cd();
    plotEventCounterTriggers->SaveCanvas(plotEventCounterTriggers->GetName());

    int nlumipanels = lumiHistsForYears.size() + 1;
    auto plotLuminosityYears = new ROOT6tools::TSavableCanvas("luminosityYears", "Integrated luminosity for different years", 400 * nlumipanels, 400);
    plotLuminosityYears->Divide(nlumipanels, 1);
    currentpad = 1;
    for(auto &[year, hist] : lumiHistsForYears) {
        plotLuminosityYears->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Integrated luminosity %d", year));
        hist->Draw();
        currentpad++;
    } 
    plotLuminosityYears->cd(currentpad);
    hLuminosityAllYears->SetStats(false);
    hLuminosityAllYears->SetTitle("Integrated luminosity all years");
    hLuminosityAllYears->Draw();
    plotLuminosityYears->cd();
    plotLuminosityYears->SaveCanvas(plotLuminosityYears->GetName());

    auto plotLuminosityTriggers = new ROOT6tools::TSavableCanvas("luminosityTriggers", "Integrated luminosity for different trigger classes", 400 * lumiHistsForTriggers.size(), 400);
    plotLuminosityTriggers->Divide(lumiHistsForTriggers.size(), 1);
    currentpad = 1;
    for(const auto & trigger : triggerorder) {
        auto hist = lumiHistsForTriggers[trigger];
        plotLuminosityTriggers->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Integrated luminosity %s", trigger.data()));
        fixYearLabels(hist);
        hist->Draw();
        currentpad++;
    } 
    plotLuminosityTriggers->cd();
    plotLuminosityTriggers->SaveCanvas(plotLuminosityTriggers->GetName());

    auto plotUncertaintyYears = new ROOT6tools::TSavableCanvas("luminosityUncertaintyYears", "Luminosity uncertainty for different years", 400 * uncertaintyHistsForYears.size(), 400);
    plotUncertaintyYears->Divide(uncertaintyHistsForYears.size(), 1);
    currentpad = 1;
    for(auto &[year, hist] : uncertaintyHistsForYears) {
        plotUncertaintyYears->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Sys. Uncertainty %d", year));
        hist->Draw();
        currentpad++;
    } 
    plotUncertaintyYears->cd();
    plotUncertaintyYears->SaveCanvas(plotUncertaintyYears->GetName());

    auto plotUncertaintyTriggers = new ROOT6tools::TSavableCanvas("luminosityUncertaintyTriggers", "Luminosity uncertainty different trigger classes", 400 * uncertaintyHistsForTriggers.size(), 400);
    plotUncertaintyTriggers->Divide(uncertaintyHistsForTriggers.size(), 1);
    currentpad = 1;
    for(const auto & trigger : triggerorder) {
        auto hist = uncertaintyHistsForTriggers[trigger];
        plotUncertaintyTriggers->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Sys. Uncertainty %s", trigger.data()));
        fixYearLabels(hist);
        hist->Draw();
        currentpad++;
    } 
    plotUncertaintyTriggers->cd();
    plotUncertaintyTriggers->SaveCanvas(plotUncertaintyTriggers->GetName());

    auto plotDownscalingYears = new ROOT6tools::TSavableCanvas("effectiveDownscalingYears", "Effective downscaling for different years", 400 * downscalingHistsForYears.size(), 400);
    plotDownscalingYears->Divide(downscalingHistsForYears.size(), 1);
    currentpad = 1;
    for(auto &[year, hist] : downscalingHistsForYears) {
        plotDownscalingYears->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Effective downscaling, %d", year));
        hist->Draw();
        currentpad++;
    } 
    plotDownscalingYears->cd();
    plotDownscalingYears->SaveCanvas(plotDownscalingYears->GetName());

    auto plotDownscalingTriggers = new ROOT6tools::TSavableCanvas("effectiveDownscalingTriggers", "Event counter for different trigger classes", 400 * downscalingHistsForTriggers.size(), 400);
    plotDownscalingTriggers->Divide(downscalingHistsForTriggers.size(), 1);
    currentpad = 1;
    for(const auto & trigger : triggerorder) {
        auto hist = downscalingHistsForTriggers[trigger];
        plotDownscalingTriggers->cd(currentpad);
        hist->SetStats(false);
        hist->SetTitle(Form("Effective downscaling %s", trigger.data()));
        fixYearLabels(hist);
        hist->Draw();
        currentpad++;
    } 
    plotDownscalingTriggers->cd();
    plotDownscalingTriggers->SaveCanvas(plotDownscalingTriggers->GetName());
}