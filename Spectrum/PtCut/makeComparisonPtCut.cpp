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

void makeComparisonPtCut(){
    std::vector<Color_t> colors = {kRed, kGreen+2, kBlue, kViolet, kOrange};
    std::vector<Double_t> jetradii = {0.2, 0.3, 0.4, 0.5};
    std::vector<int> variations = {100, 125, 150};
    auto plot = new ROOT6tools::TSavableCanvas("PtCutComparison", "pt-cut comparison", 1200, 1000);
    plot->Divide(2,2);
    int ipad = 1;
    for(auto r : jetradii){
        plot->cd(ipad);
        (new ROOT6tools::TAxisFrame(Form("swapR%02d", int(r*10.)), "p_{t} GeV/c", "Variation / p_{t}-cut 200 GeV", 0., 250, 0.5, 1.2))->Draw("axis");
        TLegend *leg(nullptr);
        if(ipad == 1){
            leg = new ROOT6tools::TDefaultLegend(0.7, 0.65, 0.89, 0.89);
            leg->Draw();
        }
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.25, 0.22, Form("R=%.1f", r)))->Draw();
        std::stringstream refname;
        refname << "corrected1DBayes_R" << std::setw(2) << std::setfill('0') << int(r*10.) << ".root";
        auto refspec = readNormalizedSpectrum(Form("default/%s", refname.str().data()));
        for(auto ivar : ROOT::TSeqI(0, variations.size())){
            std::stringstream filename;
            filename << "corrected1DBayes_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_pt" << variations[ivar] << ".root";
            auto varspec = readNormalizedSpectrum(Form("ptcut%d/%s", variations[ivar], filename.str().data()));
            varspec->Divide(refspec);
            Style{colors[ivar], static_cast<Style_t>(24+ivar)}.SetStyle<TH1>(*varspec);
            varspec->Draw("epsame");
            if(leg) leg->AddEntry(varspec, Form("ptmax %d GeV/c", variations[ivar]), "lep");
        }
        ipad++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}