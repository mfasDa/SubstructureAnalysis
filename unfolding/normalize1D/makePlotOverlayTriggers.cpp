#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

void makePlotOverlayTriggers(const std::string_view inputfile){
    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};
    std::map<std::string, TH1 *> spectra, ratios;
    std::map<std::string, Style> styles = {{"INT7", {kBlack, 20}}, {"EJ1", {kRed, 24}}, {"EJ2", {kBlue, 25}}};
    auto rstring = inputfile.substr(inputfile.find("R")+1, 2);
    double radius = float(std::stoi(std::string(rstring)))/10.;
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        for(const auto &trg : triggers) {
            auto spechist = static_cast<TH1 *>(reader->Get(Form("dataspec_R%02d_%s", int(radius*10.), trg.data())));
            spechist->SetDirectory(nullptr);
            styles[trg].SetStyle<TH1>(*spechist);
            spectra[trg] = spechist;

            if(trg != "INT7") {
                auto ratio = static_cast<TH1 *>(reader->Get(Form("%soverMB_R%02d", trg.data(), int(radius*10.))));
                ratio->SetDirectory(nullptr);
                styles[trg].SetStyle<TH1>(*ratio);
                ratios[trg] = ratio;
            }
        }
    }
    auto plot = new ROOT6tools::TSavableCanvas(Form("detcorspeccomp_jets_R%02d", int(radius*10.)), Form("Comparison detector-corrected spectra R=%.1f", radius), 1200, 600);
    plot->Divide(2,1);
    plot->cd(1);
    gPad->SetLogy();
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    (new ROOT6tools::TAxisFrame("specframe", "p_{t,det} (GeV/c)", "1/N_{ev,corr} dN_{jet}/dp_{t}", 0., 200., 1e-10, 1e-1))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.2, 0.85, 0.45, 0.89, Form("Jets, R=%.1f", radius)))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.75, 0.75, 0.94, 0.89);
    leg->Draw();
    for(const auto &trg : triggers){
        auto spec = spectra.find(trg)->second;
        spec->Draw("epsame");
        leg->AddEntry(spec, trg.data(), "lep");
    }

    plot->cd(2);
    gPad->SetLeftMargin(0.15);
    gPad->SetRightMargin(0.05);
    (new ROOT6tools::TAxisFrame("ratioframe", "p_{t,det} (GeV/c)", "Trigger / min. bias", 0., 200., 0., 2.))->Draw("axis");
    for(auto trg : triggers){
        if(trg == "INT7") continue;
        auto rathist = ratios.find(trg)->second;
        rathist->Draw("epsame"); 
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}