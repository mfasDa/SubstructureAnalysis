#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

TH1 *readNormalizedSpectrum(const std::string_view filename){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("iteration4");
    auto hist = static_cast<TH1 *>(gDirectory->Get("normalized_iter4"));
    hist->SetDirectory(nullptr);
    return hist;
}

void CompareZcutDefault(){
    std::vector<Color_t> colors = {kRed, kGreen+2, kBlue, kViolet};
    std::vector<Double_t> jetradii = {0.2, 0.3, 0.4, 0.5};
    auto plot = new ROOT6tools::TSavableCanvas("Zcutcomp", "Comparison leading z cut", 800, 600);
    (new ROOT6tools::TAxisFrame("zcomp", "p_t", "Cut / Default", 0., 250, 0.9, 1.05))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.7, 0.65, 0.89, 0.89);
    leg->Draw();
    int icase=0;
    for(auto r : jetradii){
        std::stringstream filename;
        filename << "corrected1DBayes_R" << std::setw(2) << std::setfill('0') << int(r*10.) << ".root";
        auto histdefault = readNormalizedSpectrum(Form("Default/%s", filename.str().data())),
             histzcut = readNormalizedSpectrum(Form("Zcut/%s",filename.str().data()));
        histzcut->Divide(histzcut, histdefault, 1., 1., "b");
        Style{colors[icase], static_cast<Style_t>(24+icase)}.SetStyle<TH1>(*histzcut);
        histzcut->Draw("epsame");
        leg->AddEntry(histzcut, Form("R=%.1f", r), "lep");
        icase++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}