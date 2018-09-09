#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/string.C"

void extractTriggerClusterCorrection(const std::string_view inputfile){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    auto corrhist = static_cast<TH1 *>(reader->Get("CENTNOTRDCorrection"));
    corrhist->SetDirectory(nullptr);    
    Style{kRed, 24}.SetStyle<TH1>(*corrhist);

    auto rstring = inputfile.substr(inputfile.find("R")+1, 2);
    double radius = float(std::stoi(std::string(rstring)))/10.;

    auto plot = new ROOT6tools::TSavableCanvas("LivevtimeCorrection", "Livetime correction", 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("relframe", "p_{t} (GeV/c)", "Jet Yields CENT+CENTNOTRD / CENT", 0., 200., 0.9, 1.5))->Draw("axis");
    corrhist->Draw("epsame");
    auto fit = new TF1("fit", "pol0", 20., 200.);
    corrhist->Fit(fit, "N", "", 20., 200.);
    fit->SetLineColor(kBlack);
    fit->Draw("lsame"); 
    (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.45, 0.22, Form("Jets, R=%.1f", radius)))->Draw();
    (new ROOT6tools::TNDCLabel(0.65, 0.8, 0.89, 0.89, Form("Ratio = %.2f #pm %.f", fit->GetParameter(0), fit->GetParError(0))))->Draw();
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}