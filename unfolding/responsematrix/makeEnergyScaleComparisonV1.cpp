#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

std::pair<TGraphErrors *, TGraphErrors *> readfile(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    gROOT->cd();
    auto hdiff = static_cast<TH2 *>(reader->Get("ptdiff_allptsim"));
    hdiff->RebinX(10);
    TGraphErrors *mean = new TGraphErrors, *median = new TGraphErrors, *sigma = new TGraphErrors;
    for(auto b : ROOT::TSeqI(0, hdiff->GetXaxis()->GetNbins())) {
        std::unique_ptr<TH1> slice(hdiff->ProjectionY("py", b+1, b+1));
        mean->SetPoint(b, hdiff->GetXaxis()->GetBinCenter(b+1), slice->GetMean());
        mean->SetPointError(b, hdiff->GetXaxis()->GetBinWidth(b+1)/2., slice->GetMeanError()); 
        sigma->SetPoint(b, hdiff->GetXaxis()->GetBinCenter(b+1), slice->GetRMS());
        sigma->SetPointError(b, hdiff->GetXaxis()->GetBinWidth(b+1)/2., slice->GetRMSError()); 
    }
    return {mean, sigma};
}

void makeEnergyScaleComparisonV1(){
    std::map<int, TGraphErrors *> mean, sigma;
    for(auto r : ROOT::TSeqI(2, 6)){
        std::stringstream responsefile;
        responsefile << "responsematrixpt_FullJets_R" << std::setw(2) << std::setfill('0') << r << "_INT7_merged.root";
        auto data = readfile(responsefile.str());
        mean[r] = data.first;
        sigma[r] = data.second;
    }
    const std::array<Color_t, 4> colors = {kRed, kBlue, kGreen, kViolet};

    auto plot = new ROOT6tools::TSavableCanvas("jeteneryscalefullV1", "Jet energy scale full jets", 1200, 800);
    plot->Divide(2,1);
    plot->cd(1);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.04);
    (new ROOT6tools::TAxisFrame("meanframe", "p_{t,part} (GeV/c)", "<(p_{t,det} - p_{t,part})/p_{t,part}>", 20., 200., -0.6, 0.6))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.7, 0.65, 0.89, 0.89);
    leg->Draw();
    plot->cd(2);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.04);
    (new ROOT6tools::TAxisFrame("meanframe", "p_{t,part} (GeV/c)", "#sigma((p_{t,det} - p_{t,part})/p_{t,part})", 20., 200., 0., 0.5))->Draw("axis");
    
    int icol = 0; int marker = 24;
    for(auto r : ROOT::TSeqI(2, 6)){
        Style rstyle{colors[icol], static_cast<Style_t>(marker)};
        plot->cd(1);
        auto mn = mean[r];
        rstyle.SetStyle<TGraphErrors>(*mn);
        mn->Draw("epsame");
        leg->AddEntry(mn, Form("R=%.1f", float(r)/10.), "lep");
        plot->cd(2);
        auto sg = sigma[r];
        rstyle.SetStyle<TGraphErrors>(*sg);
        sg->Draw("epsame");
        icol++;
        marker++; 
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}