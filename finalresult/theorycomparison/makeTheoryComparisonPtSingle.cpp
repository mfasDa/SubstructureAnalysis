
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/string.C"

struct PtBin {
    double              fPtmin;
    double              fPtmax;
    TGraphErrors        *fStat;
    TGraphAsymmErrors   *fSys;

    bool operator==(const PtBin &other) const { return TMath::Abs(fPtmin - other.fPtmin) < DBL_EPSILON && TMath::Abs(fPtmax - other.fPtmax) < DBL_EPSILON; }
    bool operator<(const PtBin &other) const { return fPtmax <= other.fPtmin; }

    void AddToLegend(TLegend * leg) {
        std::stringstream legentry;
        legentry << std::fixed << std::setprecision(1) << fPtmin << " GeV/c < #it{p}_{T}^{jet} < " << fPtmax  << " GeV/c";
        leg->AddEntry(this->fStat, legentry.str().data(), "lep");
    }
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
        for(auto b : ROOT::TSeqI(1, reference->GetNbinsX())){ // drop untagged bins
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
        filename << "data/SystematicsZg_R" << std::setw(2) << std::setfill('0') << int(r*10.) << "_" << t << ".root";
        auto binlimits = ptlimits.find(t)->second;
        auto trgdata = makeSysResult(filename.str(), binlimits.first, binlimits.second);
        for(auto ptb : trgdata) result.insert(ptb);
    }
    return result;
}

TGraphErrors *HistToGraph(const TH1 *hist){
    TGraphErrors *graph = new TGraphErrors;
    int np(0);
    for(auto b : ROOT::TSeqI(1, hist->GetNbinsX())){
        graph->SetPoint(np, hist->GetXaxis()->GetBinCenter(b+1), hist->GetBinContent(b+1));
        graph->SetPointError(np, hist->GetXaxis()->GetBinWidth(b+1)/2., hist->GetBinError(b+1));
        np++;
    }
    return graph;
}

std::set<PtBin> readTheory(const std::string_view theory, double r) {
    std::set<PtBin> result;
    std::stringstream filename;
    filename << "theory/" << theory << "/zgdists_" << theory << "_R" << std::setw(2) << std::setfill('0') << int(r*10.) << ".root";
    std::unique_ptr<TFile> reader(TFile::Open(filename.str().data(), "READ"));
    for(auto k : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
        if(!contains(k->GetName(), "zgpt")) continue; 
        auto hist = k->ReadObject<TH1>();        
        auto tokens = tokenize(k->GetName(), '_');
        result.insert({static_cast<double>(std::stoi(tokens[1])), static_cast<double>(std::stoi(tokens[2])), HistToGraph(hist), nullptr});
    }
    return result;
}

TGraph *makeLineGraph(TGraphErrors *input){
    TGraph *out = new TGraph;
    for(auto p : ROOT::TSeqI(0, input->GetN())) out->SetPoint(p, input->GetX()[p], input->GetY()[p]);
    return out;
}

PtBin makeRatioDataTheory(const PtBin &data, const PtBin &theory) {
    auto stat = new TGraphErrors;
    auto sys = new TGraphAsymmErrors;
    for(auto p : ROOT::TSeqI(0, data.fStat->GetN())){
        auto theoval = theory.fStat->GetY()[p];
        stat->SetPoint(p, data.fStat->GetX()[p], data.fStat->GetY()[p]/theoval);
        sys->SetPoint(p, data.fSys->GetX()[p], data.fSys->GetY()[p]/theoval);
        stat->SetPointError(p, data.fStat->GetEX()[p], data.fStat->GetEY()[p]/theoval);
        sys->SetPoint(p, data.fSys->GetX()[p], data.fSys->GetY()[p]/theoval);
        sys->SetPointError(p, data.fSys->GetEXlow()[p], data.fSys->GetEXhigh()[p], data.fSys->GetEYlow()[p]/theoval, data.fSys->GetEYhigh()[p]/theoval);
    }
    return {data.fPtmin, data.fPtmax, stat, sys};
}

void makeTheoryComparisonPtSingle(double r){
    std::array<Color_t, 3> colors = {{kBlue, kGreen+2, kRed}};
    std::array<Style_t, 3> markers = {{26, 25, 24}};
    std::vector<std::pair<double, double>> ptbins = {{160., 180.}, {60., 80.}, {30., 40.}};
    auto plot = new ROOT6tools::TSavableCanvas("zgtheorycomppt_multi", "zg vs pt", 800, 1000);
    plot->cd();
    auto distpad = new TPad(Form("distpad%d", int(r*10.)), Form("distpad %d", int(r*10.)), 0., 0.35, 1., 1.);
    distpad->Draw();
    distpad->cd();
    distpad->SetBottomMargin(0);
    distpad->SetLeftMargin(0.12);
    distpad->SetRightMargin(0.04);
    distpad->SetTopMargin(0.04);
    distpad->SetTicks();
    auto specframe = new ROOT6tools::TAxisFrame(Form("rdepframe%d", int(r*10.)), "#it{z}_{g}", "1/#it{N}_{jet} d#it{N}/d#it{z}_{g}", 0., 0.55, 0.0001, 12.);
    specframe->GetYaxis()->SetTitleSize(0.045);
    specframe->GetYaxis()->SetLabelSize(0.045);
    specframe->Draw("axis");
    TLegend *leg(nullptr), *legtheo(nullptr);
    auto jetlabel = new ROOT6tools::TNDCLabel(0.15, 0.65, 0.85, 0.92, "ALICE Preliminary, pp #sqrt{s}= 13 TeV, #int#it{L}dt = 11.5 nb^{-1} - 4 pb^{-1}");
    jetlabel->SetTextAlign(12);
    jetlabel->AddText(Form("Anti-#it{k}_{T}, R=%.1f", r));
    jetlabel->AddText("#it{p}_{T}^{track} > 0.15 GeV/c, #it{E}^{cluster}  > 0.3 GeV");
    jetlabel->AddText("|#it{#eta}^{track}| < 0.7, |#it{#eta}^{cluster}| < 0.7, |#it{#eta}^{jet}| < 0.7 - R ");
    jetlabel->AddText("SoftDrop: #it{z}_{cut} = 0.1, #it{#beta} = 0");
    jetlabel->Draw();
    leg = new ROOT6tools::TDefaultLegend(0.4, 0.45, 0.94, 0.64);
    leg->Draw();
    legtheo = new ROOT6tools::TDefaultLegend(0.45, 0.35, 0.94, 0.43);
    legtheo->Draw();
    auto rdata = readDists(r),
         rperugia = readTheory("Perugia11", r);
    int icase = 0;
    std::vector<PtBin> ratiosPerugia;
    std::set<PtBin> legentries;
    int fillstyle = 3001;
    for(auto ptbin : ptbins){
        auto bindata = rdata.find({ptbin.first, ptbin.second, nullptr, nullptr});
        auto binperugia = rperugia.find({ptbin.first, ptbin.second, nullptr, nullptr});
        Style{colors[icase], markers[icase]}.SetStyle<TGraphErrors>(*(bindata->fStat));
        bindata->fStat->Draw("epsame");
        bindata->fSys->SetLineColor(colors[icase]);
        //bindata->fSys->SetFillColor(colors[icase]);
        //bindata->fSys->SetFillStyle(3001);
        bindata->fSys->SetFillStyle(0);
        //bindata->fSys->SetFillColorAlpha(colors[icase], 0.6-(double)icase*0.2);
        bindata->fSys->Draw("2same");
        auto lineperugia = makeLineGraph(binperugia->fStat);
        lineperugia->SetLineColor(colors[icase]);
        lineperugia->SetLineStyle(1);
        lineperugia->SetLineWidth(2);
        lineperugia->Draw("lsame");
        if(legtheo && icase == 2) legtheo->AddEntry(lineperugia, "PYTHIA6, Perugia 2011", "l");
        auto ratiodataperugia = makeRatioDataTheory(*bindata, *binperugia);
        Style{colors[icase], markers[icase]}.SetStyle<TGraphErrors>(*(ratiodataperugia.fStat));
        //ratiodataperugia.fSys->SetLineColor(colors[icase]);
        ratiodataperugia.fSys->SetFillColor(colors[icase]);
        ratiodataperugia.fSys->SetFillStyle(fillstyle);
        ratiosPerugia.push_back(ratiodataperugia);
        legentries.insert(*bindata);
        //fillstyle++;
        icase++;
    }
    for(auto l : legentries) l.AddToLegend(leg);
        
        // draw ratio Data / Theory
    plot->cd();
    auto ratiopad = new TPad(Form("ratiopad%d", int(r*10.)), Form("ratiopad %d", int(r*10.)), 0., 0., 1., 0.35);
    ratiopad->Draw();
    ratiopad->cd();
    ratiopad->SetTopMargin(0);
    ratiopad->SetLeftMargin(0.12);
    ratiopad->SetRightMargin(0.04);
    ratiopad->SetBottomMargin(0.2);
    ratiopad->SetTicks();
    auto ratioframe = new ROOT6tools::TAxisFrame(Form("ratioframe%d", int(r*10.)), "z_{g}", "Data / Theory", 0., 0.55, 0.55, 1.45);
    ratioframe->GetXaxis()->SetTitleSize(0.08);
    ratioframe->GetXaxis()->SetLabelSize(0.07);
    ratioframe->GetYaxis()->SetTitleSize(0.07);
    ratioframe->GetYaxis()->SetLabelSize(0.07);
    ratioframe->GetYaxis()->SetTitleOffset(0.8);
    ratioframe->Draw("axis");
    for(auto r : ratiosPerugia) {
        r.fStat->Draw("epsame");
        r.fSys->Draw("2same");
    }

    plot->Update();
    plot->SaveCanvas(plot->GetName());
}