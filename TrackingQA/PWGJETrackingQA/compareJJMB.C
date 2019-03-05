#include "../../helpers/graphics.C"
struct period {
    std::string name;
    TH1 *effMB;
    TH1 *effJJ;

    bool operator<(const period &other) const { return name < other.name; }
    bool operator==(const period &other) const { return name == other.name; }
};

TH1 *readFile(const std::string_view fname) {
    std::unique_ptr<TFile> reader(TFile::Open(fname.data(), "READ"));
    auto eff = static_cast<TH1 *>(reader->Get("efficiency"));
    eff->SetDirectory(nullptr);
    return eff;
}

period makePeriod(const std::string_view name) {
    return { name.data(), readFile(Form("%s/trackingEfficiency_full_MB.root", name.data())), readFile(Form("%s/trackingEfficiency_full_JJ.root", name.data())) };
}

void compareJJMB(){
    auto plot = new ROOT6tools::TSavableCanvas("hybridEffPeriods", "Mybrid tracking efficiency per period", 1200, 800); 
    plot->Divide(4,2);

    Style mbstyle{kRed, 24}, jjstyle{kBlue, 25};

    std::vector<std::string> periods = {"LHC16h", "LHC16i", "LHC16j", "LHC16k", "LHC16l", "LHC16o", "LHC16p"};
    int ipad(1);
    for(auto p : periods) {
        TLegend *leg(nullptr);
        plot->cd(ipad++);
        (new ROOT6tools::TAxisFrame(Form("effframe%s", p.data()), "p_{t,part} (GeV/c)", "Tracking efficiency", 0., 60., 0., 1.1))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.75, 0.4, 0.89, p.data()))->Draw();
        if(ipad == 2) {
            leg = new ROOT6tools::TDefaultLegend(0.25, 0.15, 0.89, 0.3);
            leg->Draw();
        }

        auto data = makePeriod(p);
        mbstyle.SetStyle<TH1>(*data.effMB);
        data.effMB->Draw("psame");
        jjstyle.SetStyle<TH1>(*data.effJJ);
        data.effJJ->Draw("psame");
        if(leg){
            leg->AddEntry(data.effMB, "Min. Bias", "lep");
            leg->AddEntry(data.effJJ, "Jet-Jet LHC19a1", "lep");
        }
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}