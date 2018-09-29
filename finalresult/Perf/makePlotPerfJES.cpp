
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/substructuretree.C"

int extractR(const std::string_view filename){
    auto tokens = tokenize(std::string(filename), '_');
    return std::stoi(tokens[2].substr(1));
}

void makePlotDeltaPtSlice(const std::string_view inputfile, TLegend *leg = nullptr){
    auto radius = extractR(inputfile);

    TH2* hdiff(nullptr);
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        hdiff = static_cast<TH2 *>(reader->Get("ptdiff_allptsim"));
        hdiff->SetDirectory(nullptr);
    }

    std::vector<std::pair<double, double>> ranges = {{40., 50.}, {80., 100.}, {120., 160}};
    std::vector<Style> styles = {{kRed, 24}, {kGreen+2, 25}, {kBlue, 26}};
    auto zeroline = new TLine(0., 0.01, 0., 0.1);
    zeroline->SetLineColor(kBlack);
    zeroline->SetLineStyle(2);
    zeroline->Draw("epsame");
    int iplot = 0;
    for(auto r : ranges){
        auto hist = hdiff->ProjectionY(Form("jes_%d_%d", int(r.first), int(r.second)), hdiff->GetXaxis()->FindBin(r.first + 0.5), hdiff->GetXaxis()->FindBin(r.second - 0.5));
        hist->Scale(1./hist->Integral());
        hist->Rebin(4);
        styles[iplot++].SetStyle<TH1>(*hist);
        hist->Draw("epsame");
        if(leg) leg->AddEntry(hist, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", r.first, r.second), "lep");
    }
} 

void makePlotPerfJES(){
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    auto plot = new ROOT6tools::TSavableCanvas("PerfSlicesJES", "Slices JES", 1200, 1000);
    std::vector<TPad *> pads = { 
        new TPad("Pad0", "Pad0", 0., 0.5, 0.5, 1.), 
        new TPad("Pad1", "Pad1", 0.5, 0.5, 1., 1.), 
        new TPad("Pad2", "Pad2", 0., 0.0, 0.5, 0.5), 
        new TPad("Pad3", "Pad3", 0.5, 0., 1., 0.5)
    };
    std::vector<Color_t> colors = {kYellow, kTeal, kMagenta, kGray};

    int ipad = 0;
    for(auto r : jetradii) {
        plot->cd();
        //pads[ipad]->SetFillColor(colors[ipad]);
        pads[ipad]->Draw();
        pads[ipad]->cd();
        switch(ipad){
            case 0: {
                gPad->SetBottomMargin(0.);
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(0.13);
                gPad->SetTopMargin(0.13);
                break;
            }
            case 1 :{
                gPad->SetBottomMargin(0.);
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(0.13);
                gPad->SetTopMargin(0.13);
                break;
            }
            case 2: {
                gPad->SetTopMargin(0.);
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(0.13);
                gPad->SetBottomMargin(0.13);
                break;
            }
            case 3: {
                gPad->SetTopMargin(0.);
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(0.13);
                gPad->SetBottomMargin(0.13);
                break;
            }
        };
        gPad->SetTicks();
        auto axis = new ROOT6tools::TAxisFrame(Form("difframe_R%02d", int(r)), "(p_{t}^{jet,detector} - p_{t}^{jet,particle})/p_{t}^{jet,particle}", "Probability density    ", -1.1, 0.45, 0.0001, 0.145);
        axis->GetXaxis()->SetTitleOffset(1.2);
        axis->Draw("axis");
        TLegend *leg(nullptr);
        if(ipad == 0){
            auto simlabel = new ROOT6tools::TNDCLabel(0.15, 0.59, 0.84, 0.84, "ALICE simulation, PYTHIA8 Monash(2013), pp #sqrt{s} = 13 TeV");
            simlabel->AddText("Jets, FastJet anti-k_{t} , R=0.2");
            simlabel->AddText("p_{t}^{track} > 0.15 GeV/c, E^{cluster} > 0.3 GeV");
            simlabel->AddText("|#eta^{track,cluster}| < 0.7, |#eta^{jet}| < 0.7 - R");
            simlabel->SetTextAlign(12);
            simlabel->Draw();
        }  else {

            (new ROOT6tools::TNDCLabel(1-gPad->GetRightMargin() - 0.13, 1-gPad->GetTopMargin()-0.08, 1-gPad->GetRightMargin() - 0.04, 1-gPad->GetTopMargin() -0.03, Form("R=%.1f", r)))->Draw();
        }
        if(ipad == 1){
            leg = new ROOT6tools::TDefaultLegend(0.05, 0.55, 0.5, 0.84);
            leg->Draw();
        }
        std::stringstream filename;
        filename << "responsematrixpt_FullJets_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_INT7_merged.root"; 
        makePlotDeltaPtSlice(filename.str(), leg);
        ipad++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}