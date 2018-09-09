#include "../../meta/root.C"
#include "../../meta/root6tools.C"

void plotStatRelTriggers(const std::string_view inputfile){
    std::vector<std::string> triggers = {"EJ1", "EJ2", "INT7"};
    auto rstring = inputfile.substr(inputfile.find("R")+1, 2);
    double radius = float(std::stoi(std::string(rstring)))/10.;
    std::map<std::string, TH1 *> dataspectra;
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        for(auto trg : triggers){
            auto hist = static_cast<TH1 *>(reader->Get(Form("dataspec_R%02d_%s", int(radius * 10.), trg.data())));
            hist->SetDirectory(nullptr);
            for(auto b : ROOT::TSeqI(0, hist->GetNbinsX())){
                auto corr = hist->GetBinError(b+1)/hist->GetBinContent(b+1);
                hist->SetBinContent(b+1, corr);
                hist->SetBinError(b+1, 0);
            }
            dataspectra[trg] = hist;
        }
    }

    auto plot = new ROOT6tools::TSavableCanvas(Form("StatR%02d", int(radius*10.)), Form("Statistics R=%.1f", radius), 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("relframe", "p_{t} (GeV/c)", "rel. stat. uncertainty (%)", 0., 200., 0., 0.5))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.7, 0.3, 0.89);
    leg->Draw();
    (new ROOT6tools::TNDCLabel(0.65, 0.8, 0.89, 0.89, Form("Jets, R=%.1f", radius)))->Draw();
    auto int7spec = dataspectra.find("INT7")->second;
    int7spec->SetLineColor(kBlack);
    int7spec->SetFillStyle(0);
    int7spec->Draw("boxsame");
    leg->AddEntry(int7spec, "INT7", "lep");
    auto ej2spec = dataspectra.find("EJ2")->second;
    ej2spec->SetLineColor(kBlue);
    ej2spec->SetFillStyle(0);
    ej2spec->Draw("boxsame");
    leg->AddEntry(ej2spec, "EJ2", "lep");
    auto ej1spec = dataspectra.find("EJ1")->second;
    ej1spec->SetLineColor(kRed);
    ej1spec->SetFillStyle(0);
    ej1spec->Draw("boxsame");
    leg->AddEntry(ej1spec, "EJ1", "lep");
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}