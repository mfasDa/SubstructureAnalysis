
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/substructuretree.C"

int extractR(const std::string_view filename){
    auto tokens = tokenize(std::string(filename), '_');
    return std::stoi(tokens[2].substr(1));
}

void makePlotDeltaPtSlice(const std::string_view inputfile, double rad, TLegend *leg = nullptr){
    auto radius = extractR(inputfile);

    TH2* hdiff(nullptr);
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        hdiff = static_cast<TH2 *>(reader->Get("ptdiff_allptsim"));
        hdiff->SetDirectory(nullptr);
    }

    std::vector<std::pair<double, double>> ranges = {{40., 50.}, {80., 100.}, {120., 160}};
    std::vector<Style> styles = {{kRed, 24}, {kGreen+2, 25}, {kBlue, 26}};
    auto zeroline = new TLine(0., 0.01, 0., 0.09);
    zeroline->SetLineColor(kBlack);
    zeroline->SetLineStyle(2);
    zeroline->Draw("epsame");
    int iplot = 0;
    for(auto r : ranges){
        auto hist = hdiff->ProjectionY(Form("jes_R%02d_%d_%d", int(rad*10), int(r.first), int(r.second)), hdiff->GetXaxis()->FindBin(r.first + 0.5), hdiff->GetXaxis()->FindBin(r.second - 0.5));
        hist->Scale(1./hist->Integral());
        hist->Rebin(4);
        styles[iplot++].SetStyle<TH1>(*hist);
        hist->Draw("epsame");
        if(leg) leg->AddEntry(hist, Form("%.1f GeV/#it{c} < #it{p}_{T} < %.1f GeV/#it{c}", r.first, r.second), "lep");
    }
} 

void makePlotPerfJES(){
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    auto plot = new ROOT6tools::TSavableCanvas("PerfSlicesJES", "Slices JES", 1200, 1000);
    double xdiff = 0.534, ydiff = 0.53;
    std::vector<TPad *> pads = { 
        new TPad("Pad0", "Pad0", 0., ydiff, xdiff, 1.), 
        new TPad("Pad1", "Pad1", xdiff, ydiff, 1., 1.), 
        new TPad("Pad2", "Pad2", 0., 0.0, xdiff, ydiff), 
        new TPad("Pad3", "Pad3", xdiff, 0., 1., ydiff)
    };
    std::vector<Color_t> colors = {kYellow, kTeal, kMagenta, kGray};

    int ipad = 0;
    for(auto r : jetradii) {
        plot->cd();
        //pads[ipad]->SetFillColor(colors[ipad]);
        pads[ipad]->Draw();
        pads[ipad]->cd();
        double fullbottom = 0.17, fullleft = 0.17, fullright = 0.04, fulltop = 0.04;
        switch(ipad){
            case 0: {
                gPad->SetBottomMargin(0.);
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(fullleft);
                gPad->SetTopMargin(fulltop);
                break;
            }
            case 1 :{
                gPad->SetBottomMargin(0.);
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(fullright);
                gPad->SetTopMargin(fulltop);
                break;
            }
            case 2: {
                gPad->SetTopMargin(0.);
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(fullleft);
                gPad->SetBottomMargin(fullbottom);
                break;
            }
            case 3: {
                gPad->SetTopMargin(0.);
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(fullright);
                gPad->SetBottomMargin(fullbottom);
                break;
            }
        };
        gPad->SetTicks();
        auto axis = new ROOT6tools::TAxisFrame(Form("difframe_R%02d", int(r)), "(#it{p}_{T,jet}^{det} - #it{p}_{T,jet}^{part})/#it{p}_{T,jet}^{part}", "Probability density    ", -1.1, 0.45, 0.0001, 0.145);
        axis->GetXaxis()->SetTitleOffset(1.2);
        axis->GetXaxis()->SetTitleSize(0.055);
        axis->GetXaxis()->SetLabelSize(0.055);
        axis->GetYaxis()->SetLabelSize(0.055);
        axis->GetYaxis()->SetTitleSize(0.055);
        axis->GetXaxis()->CenterTitle();
        axis->GetYaxis()->CenterTitle();
        axis->Draw("axis");
        TLegend *leg(nullptr);
        if(ipad == 0){
            auto simlabel = new ROOT6tools::TNDCLabel(0.2, 0.63, 0.94, 0.94, "pp #sqrt{s} = 13 TeV, PYTHIA8(Monash2013)");
            simlabel->AddText("Anti-#it{k}_{T} , #it{R} = 0.2");
            simlabel->AddText("#it{p}_{T}^{track} > 0.15 GeV/#it{c}, #it{E}^{cluster} > 0.3 GeV");
            simlabel->AddText("|#it{#eta}^{track,cluster}| < 0.7, |#it{#eta}^{jet}| < 0.7 - #it{R}");
            simlabel->SetTextAlign(12);
            simlabel->SetTextSize(0.055);
            simlabel->Draw();

            auto alicelabel = new ROOT6tools::TNDCLabel(0.2, 0.5, 0.5, 0.64, "ALICE Simulation");
            alicelabel->SetTextSize(0.055);
            alicelabel->Draw();
        }  else {

            auto rlabel = new ROOT6tools::TNDCLabel(1-gPad->GetRightMargin() - 0.23, 1-gPad->GetTopMargin()-0.18, 1-gPad->GetRightMargin() - 0.04, 1-gPad->GetTopMargin() -0.03, Form("#it{R} = %.1f", r));
            rlabel->SetTextSize(0.05);
            rlabel->Draw();
        }
        if(ipad == 1){
            leg = new ROOT6tools::TDefaultLegend(0.05, 0.65, 0.5, 0.94);
            leg->SetTextSize(0.055);
            leg->Draw();
        }
        std::stringstream filename;
        filename << "responsematrixpt_FullJets_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_INT7_merged.root"; 
        makePlotDeltaPtSlice(filename.str(), r, leg);
        ipad++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}