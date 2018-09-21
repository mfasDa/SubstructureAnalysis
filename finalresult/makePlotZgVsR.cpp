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

void makePlotZgVsR(){
    std::map<double, std::set<PtBin>> dists;
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    std::vector<std::pair<double, double>> ptbins = {{30., 40.}, {60., 80.}, {160., 180.}};
    std::map<double, Color_t> colors = {{0.2, kRed}, {0.3, kGreen+2}, {0.4, kBlue}, {0.5, kViolet}};
    std::map<double, Style_t> markers = {{0.2, 24}, {0.3, 25}, {0.4, 26}, {0.5, 27}};
    for(auto r : jetradii) dists[r] = readDists(r);
    auto plot = new ROOT6tools::TSavableCanvas("zgvsR_multiplanel", "zg vs R", 1400, 600);
    plot->Divide(3,1);
    int ipad = 1;
    for(auto ptb : ptbins) {
        plot->cd(ipad++);
        (new ROOT6tools::TAxisFrame(Form("rdepframe_%d_%d", int(ptb.first), int(ptb.second)), "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.55, 0., 8.))->Draw("axis");
        TLegend *leg(nullptr);
        if(ipad == 2){
            (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.55, 0.87, "pp, #sqrt{s}= 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
            //(new ROOT6tools::TNDCLabel(0.15, 0.80, 0.85, 0.87, "ALICE preliminary, pp, #sqrt{s}= 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
            (new ROOT6tools::TNDCLabel(0.15, 0.73, 0.35, 0.8, "jets, anti-k_{t}"))->Draw();

            leg = new ROOT6tools::TDefaultLegend(0.7, 0.55, 0.89, 0.79);
            leg->Draw();
        }
        (new ROOT6tools::TNDCLabel(0.25, 0.15, 0.7, 0.22, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptb.first, ptb.second)))->Draw();
        for(auto r : jetradii){
            auto ptbin = dists.find(r)->second.find({ptb.first, ptb.second, nullptr, nullptr});
            Style{colors[r], markers[r]}.SetStyle<TGraphErrors>(*ptbin->fStat);
            ptbin->fStat->Draw("epsame");
            if(leg){
                std::stringstream legentry;
                legentry << "R=" << std::fixed << std::setprecision(1) << r;
                leg->AddEntry(ptbin->fStat, legentry.str().data(), "lep");
            }
            ptbin->fSys->SetFillColor(colors[r]);
            ptbin->fSys->SetFillStyle(3001);
            ptbin->fSys->Draw("2same");
        }
    }
    
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}