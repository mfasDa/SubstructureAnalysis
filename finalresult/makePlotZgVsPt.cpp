#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../helpers/string.C"

struct PtBin {
    double              fPtmin;
    double              fPtmax;
    TGraphErrors        *fStat;
    TGraphAsymmErrors   *fSys;

    bool operator==(const PtBin &other) const { return TMath::Abs(fPtmin - other.fPtmin) < DBL_EPSILON && TMath::Abs(fPtmax - other.fPtmax) < DBL_EPSILON; }
    bool operator<(const PtBin &other) const { return fPtmax <= other.fPtmin; }
};

std::pair<double, double> decodePtDir(const std::string_view dirname) {
    auto tokens = tokenize(std::string(dirname), '_');
    return {double(std::stoi(tokens[1])), double(std::stoi(tokens[2]))};
}

std::set<PtBin> makeSysResult(const std::string_view inputfile, double ptmin, double ptmax){
    std::set<PtBin> result;

    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    for(auto k : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
        auto binlimits = decodePtDir(k->GetName());
        if(binlimits.second < ptmin + 0.5) continue;
        if(binlimits.first > ptmax - 0.5) break;
        reader->cd(k->GetName());

        TGraphErrors *stat = new TGraphErrors;
        TGraphAsymmErrors *sys = new TGraphAsymmErrors;
        std::unique_ptr<TH1> reference(static_cast<TH1 *>(gDirectory->Get("reference")));
        reference->SetDirectory(nullptr);
        gDirectory->cd("sum");
        std::unique_ptr<TH1> min(static_cast<TH1 *>(gDirectory->Get("lowsysrel"))), 
                            max(static_cast<TH1 *>(gDirectory->Get("upsysrel")));

        int np(0);
        for(auto b : ROOT::TSeqI(1, reference->GetNbinsX())){
            stat->SetPoint(np, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
            sys->SetPoint(np, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
            stat->SetPointError(np, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinError(b+1));
            sys->SetPointError(np, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinContent(b+1) * TMath::Abs(min->GetBinContent(b+1)), reference->GetBinContent(b+1) * TMath::Abs(max->GetBinContent(b+1)));
            np++;
        }
        result.insert({binlimits.first, binlimits.second, stat, sys});
    }

    return result;
}

std::set<PtBin> readDists(double r) {
    std::map<std::string, std::pair<double, double>> ptlimits = {{"INT7", {30., 80.}}, {"EJ2", {80., 120.}}, {"EJ1", {120., 240.}}};
    std::vector<std::string> triggers = {"INT7", "EJ2", "EJ1"};
    std::set<PtBin> result;
    for(auto t : triggers) {
        std::stringstream filename;
        filename << "SystematicsZg_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_" << t << ".root";
        auto binlimits = ptlimits.find(t)->second;
        auto trgdata = makeSysResult(filename.str(), binlimits.first, binlimits.second);
        for(auto ptb : trgdata) result.insert(ptb);
    }
    return result;
}

void makePlotZgVsPt(){
    std::array<Color_t, 3> colors = {{kRed, kGreen+2, kBlue}};
    std::array<Style_t, 3> markers = {{24, 25, 26}};
    std::vector<std::pair<double, double>> ptbins = {{30., 40.}, {60., 80.}, {140., 160.}};
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    auto plot = new ROOT6tools::TSavableCanvas("zgvspt_multi", "zg vs pt", 1200, 1200);
    plot->Divide(2,2);
    int ipanel = 1;
    for(auto r : jetradii) {
        plot->cd(ipanel++);
        (new ROOT6tools::TAxisFrame("rdepframe", "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.55, 0., 8.))->Draw("axis");
        TLegend *leg(nullptr);
        if(ipanel == 2){
            //(new ROOT6tools::TNDCLabel(0.15, 0.8, 0.85, 0.87, "ALICE preliminary, pp, #sqrt{s} = 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
            (new ROOT6tools::TNDCLabel(0.15, 0.82, 0.5, 0.87, "pp, #sqrt{s} = 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
            (new ROOT6tools::TNDCLabel(0.15, 0.75, 0.36, 0.8, Form("jets, anti-k_{t}")))->Draw();
            leg = new ROOT6tools::TDefaultLegend(0.4, 0.6, 0.89, 0.79);
            leg->Draw();
        }
        (new ROOT6tools::TNDCLabel(0.25, 0.18, 0.4, 0.23, Form("R=%.1f", r)))->Draw();
        auto rdata = readDists(r);
        int icase = 0;
        for(auto ptbin : ptbins){
            std::stringstream legentry;
            legentry << std::fixed << std::setprecision(1) << ptbin.first << " GeV/c < p_{t} < " << ptbin.second  << " GeV/c";
            auto bindata = rdata.find({ptbin.first, ptbin.second, nullptr, nullptr});
            Style{colors[icase], markers[icase]}.SetStyle<TGraphErrors>(*(bindata->fStat));
            bindata->fStat->Draw("epsame");
            if(leg) leg->AddEntry(bindata->fStat, legentry.str().data(), "lep");
            bindata->fSys->SetFillColor(colors[icase]);
            bindata->fSys->SetFillStyle(3001);
            bindata->fSys->Draw("2same");
            icase++;
        }
    }

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}