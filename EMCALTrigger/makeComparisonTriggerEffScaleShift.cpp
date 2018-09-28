#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

TH1 *EJ1triggerEff(const std::string_view filename, double r){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("rawlevel");
    auto eff = static_cast<TH1 *>(gDirectory->Get(Form("Efficiency_R%02d_EJ1", int(r*10.))));
    eff->SetDirectory(nullptr);
    return eff;
}

void makeComparisonTriggerEffScaleShift(){
    std::vector<int> scaleshifts = {25, 50, 100, 200};
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    std::map<int, Style> styles = {{25, {kRed, 24}}, {50, {kBlue, 25}}, {100, {kOrange, 26}}, {200, {kViolet, 27}}};

    auto plot = new ROOT6tools::TSavableCanvas("effscaleshiftEJ1", "Trigger efficiency for different scale shifts", 1200, 1000);
    plot->Divide(2,2);
    for(auto irad : ROOT::TSeqI(0, 4)) {
        plot->cd(irad+1);
        auto radius = jetradii[irad];
        (new ROOT6tools::TAxisFrame(Form("efframeR%02d",int(radius*10)), "p_{t} (GeV/c)", "Trigger efficiency", 0., 200., 0., 1.2))->Draw();
        TLegend *leg(nullptr);
        if(irad == 0) {
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.4);
            leg->Draw();
        }
        for(auto s : scaleshifts) {
            auto eff = EJ1triggerEff(Form("20180925_trgscale%d/corrected1DBayes_R%02d.root", s, int(radius*10.)), radius);
            styles[s].SetStyle<TH1>(*eff);
            eff->Draw("epsame");
            if(leg) leg->AddEntry(eff, Form("%d MeV scaleshift", s), "lep");
        }
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}