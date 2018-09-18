#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../helpers/string.C"

struct PtBin {
    double ptmin;
    double ptmax;
    TGraphErrors        *stat;
    TGraphAsymmErrors   *sys;

    bool operator==(const PtBin &other) const { return TMath::Abs(ptmin - other.ptmin) < DBL_EPSILON && TMath::Abs(ptmax - other.ptmax) < DBL_EPSILON; }
    bool operator<(const PtBin &other) const { return ptmax <= other.ptmin; }
};

std::pair<double, double> decodePtDir(const std::string_view dirname) {
    auto tokens = tokenize(std::string(dirname), '_');
    return {double(std::stoi(tokens[1])), double(std::stoi(tokens[2]))};
}

std::set<PtBin> makeSysResult(const std::string_view inputfile, double ptmin, double ptmax){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    std::set<PtBin> result;
    for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())) {
        if(!contains(k->GetName(), "pt")) continue;
        auto binlimits = decodePtDir(k->GetName());
        if(binlimits.first < ptmin) continue;
        if(binlimits.second > ptmax) continue;

        reader->cd(k->GetName());
        std::unique_ptr<TH1> reference(static_cast<TH1 *>(gDirectory->Get("reference")));
        reference->SetDirectory(nullptr);
        gDirectory->cd("sum");
        std::unique_ptr<TH1> min(static_cast<TH1 *>(gDirectory->Get("lowsysrel"))), 
                         max(static_cast<TH1 *>(gDirectory->Get("upsysrel")));

        TGraphErrors *stat = new TGraphErrors;
        TGraphAsymmErrors *sys = new TGraphAsymmErrors;
        for(auto b : ROOT::TSeqI(0, reference->GetNbinsX())){
            stat->SetPoint(b, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
            sys->SetPoint(b, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
            stat->SetPointError(b, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinError(b+1));
            sys->SetPointError(b, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinContent(b+1) * TMath::Abs(min->GetBinContent(b+1)), reference->GetBinContent(b+1) * TMath::Abs(max->GetBinContent(b+1)));
        }
        result.insert({binlimits.first, binlimits.second, stat, sys});
    }

    return result;
}

void makeMultipanelPlot(){
    std::map<double, std::set<PtBin>> results;
    std::map<std::string, std::pair<double, double>> ptlimits = {{"INT7", {30., 80.}}, {"EJ2", {80., 120.}}, {"EJ1", {120., 200.}}};
    for(auto r : ROOT::TSeqI(2, 6)) {
        std::set<PtBin> combinedResult;
        for(const auto &t : ptlimits) {
            std::stringstream filename;
            filename << "SystematicsZg_R" << std::setw(2) << std::setfill('0') << r << "_" << t.first << ".root";
            auto trgresults = makeSysResult(filename.str(), t.second.first, t.second.second);
            for(const auto &b : trgresults) combinedResult.insert(b);
        }
        results[double(r)/10.] = combinedResult;
    }
    auto plot = new ROOT6tools::TSavableCanvas("zgvsptvsR", "zg vs pt and R", 1200, 1000);
    plot->DivideSquare(results[0.2].size());
    std::map<double, std::set<PtBin>::iterator> iterators;
    for(auto &r : results) iterators[r.first] = r.second.begin();
    std::map<double, Style> rstyles = {{0.2, {kRed, 24}}, {0.3, {kGreen+2, 25}}, {0.4, {kBlue, 26}} , {0.5, {kViolet, 27}}};
    for(auto ipt : ROOT::TSeqI(0, results[0.2].size())) {
        plot->cd(ipt+1);
        (new ROOT6tools::TAxisFrame(Form("rdepframe_%d_%d", int(iterators[0.2]->ptmin), int(iterators[0.2]->ptmax)), "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.55, 0., 10.))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.85, 0.89, Form("%.1f GeV./c < p_{t} < %.1f GeV/c", iterators[0.2]->ptmin, iterators[0.2]->ptmax)))->Draw();
        TLegend *leg(nullptr);
        if(ipt == 0) {
            (new ROOT6tools::TNDCLabel(0.15, 0.12, 0.65, 0.17, "pp, #sqrt{s} = 13 TeV, jets, anti-k_{t}"))->Draw();
            leg = new ROOT6tools::TDefaultLegend(0.6, 0.45, 0.89, 0.78);
            leg->Draw();
        }
        for(auto &r : iterators) {
            auto stat = r.second->stat;
            auto sys = r.second->sys;
            auto rstyle = rstyles[r.first];
            rstyle.SetStyle<TGraphErrors>(*stat);
            sys->SetFillColor(rstyle.color);
            sys->SetFillStyle(3001);
            stat->Draw("epsame");
            sys->Draw("2same");
            if(leg) leg->AddEntry(stat, Form("R=%.1f", r.first), "lep");
            r.second++;
        }
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}