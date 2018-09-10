#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

void makePlotTriggerEfficiency(const std::string_view inputfile){
    std::vector<std::string> triggers = {"EJ1", "EJ2"};
    std::map<std::string, TH1 *> efficiencies;
    std::map<std::string, Style> styles = {{"EJ1", {kRed, 24}}, {"EJ2", {kBlue, 25}}};
    auto rstring = inputfile.substr(inputfile.find("R")+1, 2);
    double radius = float(std::stoi(std::string(rstring)))/10.;
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        for(const auto &trg : triggers) {
            auto effhist = static_cast<TH1 *>(reader->Get(Form("Efficiency_R%02d_%s", int(radius*10.), trg.data())));
            effhist->SetDirectory(nullptr);
            styles[trg].SetStyle<TH1>(*effhist);
            efficiencies[trg] = effhist;
        }
    }
    auto plot = new ROOT6tools::TSavableCanvas(Form("triggereff_jets_R%02d", int(radius*10.)), Form("Trigger efficiency R=%.1f", radius), 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("effframe", "p_{t,det} (GeV/c)", "Trigger efficiency", 0., 200., 0., 1.))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.65, 0.32, 0.85, 0.38, Form("Jets, R=%.1f", radius)))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.3);
    leg->Draw();
    for(const auto &trg : triggers){
        auto eff = efficiencies.find(trg)->second;
        eff->Draw("epsame");
        leg->AddEntry(eff, trg.data(), "lep");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}