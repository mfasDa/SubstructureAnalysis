#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/string.C"

struct ptbin {
    double fPtMin;
    double fPtMax;
    TH1 *fTrend;

    bool operator==(const ptbin &other) const { return fPtMin == other.fPtMin && fPtMax == other.fPtMax; }
    bool operator<(const ptbin &other) const { return fPtMax <= other.fPtMin; }
};

void trendRelativeLHC16k_efficiencyCorrected(const std::string_view inputfile){
    std::set<ptbin> trends;
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())){
        auto hist = k->ReadObject<TH1>();
        hist->SetDirectory(nullptr);
        auto periodbin = hist->GetXaxis()->FindBin("LHC16k");
        hist->Scale(1./hist->GetBinContent(periodbin));
        auto tokens = tokenize(k->GetName(), '_');
        trends.insert({double(std::stoi(tokens[2]))/10., double(std::stoi(tokens[3]))/10., hist});
    }    
    TString filestring(inputfile.data());
    filestring.ReplaceAll(".root", "");
    auto settings = tokenize(filestring.Data(), '_');

    auto plot = new ROOT6tools::TSavableCanvas(Form("trackTrendingEffCorrRelativeLHC16k_%s_%s_%s", settings[1].data(), settings[2].data(), settings[3].data()), "Track trending relative to LHC16k", 1200, 1000);
    plot->Divide(3,2);
    int ipad(1);
    for(const auto &t : trends){
        plot->cd(ipad);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        t.fTrend->GetYaxis()->SetTitle("Per-event yield relative to LHC16k");
        t.fTrend->GetYaxis()->SetRangeUser(0.85, 1.2);
        t.fTrend->Draw("ep");
        if(ipad == 1)
            (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.89, 0.89, Form("%s, %s tracks, %s acceptance", settings[2].data(), settings[1].data(), settings[3].data())))->Draw();
        ipad++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}