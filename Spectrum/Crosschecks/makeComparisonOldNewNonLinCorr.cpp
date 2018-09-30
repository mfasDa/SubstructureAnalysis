#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/root.C"

std::vector<std::string> kTriggers = {"INT7", "EJ1", "EJ2"};

std::map<std::string, TH1 *> readNormalizedRaw(const std::string_view filename, double radius) {
    std::map<std::string, TH1 *> spectre;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("rawlevel");
    for(const auto &t : kTriggers) {
        auto hist = static_cast<TH1 *>(gDirectory->Get(Form("dataspec_R%02d_%s", int(radius*10.), t.data())));
        hist->SetDirectory(nullptr);
        spectre[t] = hist;
    }
    return spectre;
}

void makeComparisonOldNewNonLinCorr(){
    std::map<double, std::map<std::string, TH1 *>> newcorr, oldcorr;
    for(auto r = 0.2; r <= 0.5; r+=0.1) {
        std::stringstream oldfile, newfile;
        oldfile << "default/corrected1DBayes_R" << std::setw(2) << std::setfill('0') << int(r*10.) << ".root";
        newfile << "corrected_NonLinCorr/corrected1DBayes_R" << std::setw(2) << std::setfill('0') << int(r*10.) << ".root";
        newcorr[r] = readNormalizedRaw(newfile.str(), r);
        oldcorr[r] = readNormalizedRaw(oldfile.str(), r);
    }
    
    for(const auto &t : kTriggers) {
        auto plot = new ROOT6tools::TSavableCanvas(Form("nonLinCorrComparison%s", t.data()), Form("Non-LinearityComparison %s", t.data()), 1200, 800);
        plot->Divide(4,2);

        std::map<std::string, Style> styles = {{"old", {kRed, 24}}, {"new", {kBlue, 25}}, {"ratio", {kBlack, 20}}};
        int icol = 0;
        for(auto r = 0.2; r <= 0.5; r+=0.1){
            plot->cd(icol+1);
            gPad->SetLogy();
            (new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(r*10.)), "p_{t} (GeV/c)", "1/N_{ev} dN/dp_{t} (GeV/c^{-1})", 0. , 250, 1e-10, 1))->Draw("axis");
            TLegend *leg(nullptr);
            if(icol == 0) {
                leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
                leg->Draw();
                (new ROOT6tools::TNDCLabel(0.65, 0.45, 0.89, 0.89, Form("Trigger: %s", t.data())))->Draw();
            }
            (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.3, 0.22, Form("R=%.1f", r)))->Draw();
            const auto &rmapold = oldcorr.find(r)->second,
                       &rmapnew = newcorr.find(r)->second;
            auto rhistold = rmapold.find(t)->second,
                 rhistnew = rmapnew.find(t)->second,
                 ratio = histcopy(rhistnew);
            styles.find("new")->second.SetStyle<TH1>(*rhistnew);
            styles.find("old")->second.SetStyle<TH1>(*rhistold);
            rhistold->Draw("epsame");
            rhistnew->Draw("epsame");

            plot->cd(icol+5);
            (new ROOT6tools::TAxisFrame(Form("ratioframeR%02d", int(r*10.)), "p_{t} (GeV/c)", "New / Old", 0. , 250, 0.5, 1.5))->Draw("axis");
            ratio->SetDirectory(nullptr);
            ratio->Divide(rhistold);
            styles.find("ratio")->second.SetStyle<TH1>(*ratio);
            ratio->Draw("epsame");

            icol++;
        }
        plot->cd();
        plot->Update();
        plot->SaveCanvas(plot->GetName());
    }
}