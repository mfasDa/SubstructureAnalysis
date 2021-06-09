#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"

struct histos {
    TH1 *hPt;
    TH1 *hTrials;
};

std::map<std::string, histos> readFile(const char *filename){
    std::map<std::string, histos> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    for(auto content : TRangeDynCast<TKey>(reader->GetListOfKeys())){
        if(!content) continue;
        TString dirname(content->GetName());
        if(!(dirname.Contains("EnergyScaleResults") || dirname.Contains("SoftDropResponse") || (dirname.Contains("JetSpectrum") && dirname.Contains("INT7")))) continue;
        reader->cd(dirname);
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto pthist = static_cast<TH1 *>(histlist->FindObject("fHistPtHard")),
             ntrialshist = static_cast<TH1 *>(histlist->FindObject("fHistTrials"));
        pthist->SetDirectory(nullptr);
        ntrialshist->SetDirectory(nullptr);
        pthist->SetStats(false);
        ntrialshist->SetStats(false);
        result[dirname.Data()] = {pthist, ntrialshist};
    }

    return result;
}

void checkPtHardSample(const char *filename = "AnalysisResults.root") {

    auto style = [](Color_t col, Style_t mrk) {
        return [col, mrk](auto hist) {
            hist->SetMarkerColor(col);
            hist->SetLineColor(col);
            hist->SetMarkerStyle(mrk);
        };
    };

    std::vector<Color_t> Colors = {kRed, kBlue, kOrange, kGreen, kMagenta, kGray, kViolet};
    std::vector<Style_t> Markers = {24, 25, 26, 27};
    auto currentcolor = Colors.begin();
    auto currentmarker = Markers.begin();

    auto plot = new ROOT6tools::TSavableCanvas("checkPtHardProd", "Check weighting histograms for pt-hard production", 1200, 800);
    plot->Divide(2,1);

    plot->cd(1);
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame("pthardframe", "p_{t,h} (GeV/c)", "d#sigma/dp_{t,h} (mb/(GeV/c))", 0., 1000, 1e-12, 100))->Draw("axis");
    auto ptleg = new ROOT6tools::TDefaultLegend(0.5, 0.3, 0.89, 0.89);
    ptleg->Draw();

    bool first = true;
    for(auto hist : readFile(filename)) {
        plot->cd(1);
        style(*currentcolor, *currentmarker)(hist.second.hPt);
        hist.second.hPt->Draw("epsame");
        ptleg->AddEntry(hist.second.hPt, hist.first.data(), "lep");
        plot->cd(2);
        style(*currentcolor, *currentmarker)(hist.second.hTrials);
        std::string plotstring = first ? "ep" : "epsame";
        hist.second.hTrials->Draw(plotstring.data());
        if(first) first = false;
        currentcolor++;
        currentmarker++;
        if(currentcolor == Colors.end()) currentcolor = Colors.begin();
        if(currentmarker == Markers.end()) currentmarker = Markers.begin();
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}