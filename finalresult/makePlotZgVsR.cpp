#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

std::pair<TGraphErrors *, TGraphAsymmErrors *> makeSysResult(const std::string_view inputfile){
    TGraphErrors *stat = new TGraphErrors;
    TGraphAsymmErrors *sys = new TGraphAsymmErrors;

    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    reader->cd("pt_40_60");
    std::unique_ptr<TH1> reference(static_cast<TH1 *>(gDirectory->Get("reference")));
    reference->SetDirectory(nullptr);
    gDirectory->cd("sum");
    std::unique_ptr<TH1> min(static_cast<TH1 *>(gDirectory->Get("lowsysrel"))), 
                         max(static_cast<TH1 *>(gDirectory->Get("upsysrel")));

    for(auto b : ROOT::TSeqI(0, reference->GetNbinsX())){
        stat->SetPoint(b, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
        sys->SetPoint(b, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
        stat->SetPointError(b, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinError(b+1));
        sys->SetPointError(b, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinContent(b+1) * TMath::Abs(min->GetBinContent(b+1)), reference->GetBinContent(b+1) * TMath::Abs(max->GetBinContent(b+1)));
    }

    return {stat, sys};
}

void makePlotZgVsR(){
    auto plot = new ROOT6tools::TSavableCanvas("zgvsR_40_60", "zg vs R", 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("rdepframe", "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.55, 0., 0.6))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.82, 0.45, 0.87, "pp, #sqrt{s}= 13 TeV"))->Draw();
    (new ROOT6tools::TNDCLabel(0.15, 0.75, 0.45, 0.81, "jets, anti-k_{t}, 40 GeV/c < p_{t} < 60 GeV/c"))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.7, 0.65, 0.89, 0.89);
    leg->Draw();
    std::map<int, Color_t> colors = {{2, kRed}, {3, kGreen+2}, {4, kBlue}, {5, kViolet}};
    std::map<int, Style_t> markers = {{2, 24}, {3, 25}, {4, 26}, {5, 27}};
    
    for(auto r : ROOT::TSeqI(2, 6)){
        std::stringstream filename, legentry;
        filename << "SystematicsZg_R" << std::setw(2) << std::setfill('0') << r << "_INT7.root";
        legentry << "R=" << std::fixed << std::setprecision(1) << double(r)/10.;
        auto results = makeSysResult(filename.str());
        Style{colors[r], markers[r]}.SetStyle<TGraphErrors>(*results.first);
        results.first->Draw("epsame");
        leg->AddEntry(results.first, legentry.str().data(), "lep");
        results.second->SetFillColor(colors[r]);
        results.second->SetFillStyle(3001);
        results.second->Draw("2same");
    }

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}