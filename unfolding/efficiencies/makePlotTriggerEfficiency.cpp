#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../struct/GraphicsPad.cxx"

struct TriggerEff{
    TH1 *fEJ1;
    TH1 *fEJ2;
};

std::map<double, TriggerEff> ReadFile(const std::string_view filename) {
    std::map<double, TriggerEff> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(auto rkey : *gDirectory->GetListOfKeys()){
        std::string rstring(rkey->GetName());
        double rval = double(std::stoi(rstring.substr(1)))/10.;
        reader->cd(rstring.data());
        gDirectory->cd("rawlevel");
        auto hEJ1 = static_cast<TH1 *>(gDirectory->Get(Form("TriggerEfficiency_EJ1_%s", rstring.data()))),
             hEJ2 = static_cast<TH1 *>(gDirectory->Get(Form("TriggerEfficiency_EJ2_%s", rstring.data())));
        hEJ1->SetDirectory(nullptr);
        hEJ2->SetDirectory(nullptr);
        result[rval] = {hEJ1, hEJ2};
    }
    return result;
}

void makePlotTriggerEfficiency(const std::string_view filename){
    auto data = ReadFile(filename);
    
    auto plot = new ROOT6tools::TSavableCanvas("TriggerEfficiency", "Trigger efficiency", 1200, 800);
    plot->Divide(3,2);

    Style ej1style{kRed, 24}, ej2style{kBlue, 25};
    int ipad = 1;
    for(const auto &[r, effdata] : data){
        plot->cd(ipad);
        GraphicsPad effpad(gPad);
        effpad.Frame(Form("effframeR%02d", int(r*10.)), "p_{t,det} (GeV/c)", "Trigger efficiency", 0., 300., 0., 1.1);
        effpad.Label(0.15, 0.8, 0.45, 0.89, Form("Full jets, R=%.1f", r));
        if(ipad == 1) effpad.Legend(0.7, 0.15, 0.89, 0.3);
        effpad.Draw<TH1>(effdata.fEJ1, ej1style, "EJ1");
        effpad.Draw<TH1>(effdata.fEJ2, ej2style, "EJ2");
        ipad++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}