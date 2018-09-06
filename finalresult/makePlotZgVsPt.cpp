#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

std::pair<TGraphErrors *, TGraphAsymmErrors *> makeSysResult(const std::string_view inputfile, double ptmin, double ptmax){
    TGraphErrors *stat = new TGraphErrors;
    TGraphAsymmErrors *sys = new TGraphAsymmErrors;

    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    reader->cd(Form("pt_%d_%d", int(ptmin), int(ptmax)));
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

void makePlotZgVsPt(double r){
    auto plot = new ROOT6tools::TSavableCanvas(Form("zgvspt_R%02d", int(r *10.)), Form("zg vs pt R=%.1f", r), 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("rdepframe", "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.55, 0., 0.6))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.82, 0.45, 0.87, "pp, #sqrt{s} = 13 TeV"))->Draw();
    (new ROOT6tools::TNDCLabel(0.15, 0.75, 0.45, 0.81, Form("jets, anti-k_{t}, R=%.1f", r)))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.6, 0.7, 0.89, 0.89);
    leg->Draw();
    std::array<Color_t, 3> colors = {{kRed, kGreen+2, kBlue}};
    std::array<Style_t, 3> markers = {{24, 25, 26}};
    std::vector<std::pair<double, double>> ptbins = {{30., 40.}, {40., 60.}, {60., 80.}};
    int icase = 0;
    std::stringstream filename;
    filename << "SystematicsZg_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_INT7.root";
    for(auto ptbin : ptbins){
        std::stringstream legentry;
        legentry << std::fixed << std::setprecision(1) << ptbin.first << " GeV/c < p_{t} < " << ptbin.second  << " GeV/c";
        auto results = makeSysResult(filename.str(), ptbin.first, ptbin.second);
        Style{colors[icase], markers[icase]}.SetStyle<TGraphErrors>(*results.first);
        results.first->Draw("epsame");
        leg->AddEntry(results.first, legentry.str().data(), "lep");
        results.second->SetFillColor(colors[icase]);
        results.second->SetFillStyle(3001);
        results.second->Draw("2same");
        icase++;
    }

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}