#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

std::vector<double> kJetRadii = {0.2, 0.3, 0.4, 0.5};

struct DataRBin {
    double              fRadius;
    TGraphErrors        *fStat;
    TGraphAsymmErrors   *fSys;

    bool operator==(const DataRBin &other) const { return TMath::Abs(fRadius - other.fRadius) < DBL_EPSILON; }
    bool operator<(const DataRBin &other) const { return fRadius < other.fRadius; } 
};

struct TheoryRBin {
    double              fRadius;
    TGraphErrors        *fData;

    bool operator==(const TheoryRBin &other) const { return TMath::Abs(fRadius - other.fRadius) < DBL_EPSILON; }
    bool operator<(const TheoryRBin &other) const { return fRadius < other.fRadius; }
};

DataRBin readDataSpectrum(const std::string_view inputfile, double radius){
    TGraphErrors *stat = new TGraphErrors;
    TGraphAsymmErrors *sys = new TGraphAsymmErrors;

    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    reader->cd();
    std::unique_ptr<TH1> reference(static_cast<TH1 *>(gDirectory->Get("reference")));
    reference->SetDirectory(nullptr);
    gDirectory->cd("sum");
    std::unique_ptr<TH1> min(static_cast<TH1 *>(gDirectory->Get("lowsysrel"))), 
                         max(static_cast<TH1 *>(gDirectory->Get("upsysrel")));

    const double kSizeEmcalPhi = 1.88,
                 kSizeEmcalEta = 1.4;
    double acceptance = (kSizeEmcalPhi - 2 * radius) * (kSizeEmcalEta - 2 * radius) / (TMath::TwoPi());
    double crosssection = 57.8;
    double epsilon_vtx = 0.8228; // for the moment hard coded, for future analyses determined automatically from the output
    reference->Scale(crosssection*epsilon_vtx/acceptance);

    int np(0);
    for(auto b : ROOT::TSeqI(0, reference->GetNbinsX())){
        if(reference->GetXaxis()->GetBinUpEdge(b+1) > 250.) break;
        if(reference->GetXaxis()->GetBinLowEdge(b+1) <30.) continue;
        stat->SetPoint(np, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
        sys->SetPoint(np, reference->GetXaxis()->GetBinCenter(b+1), reference->GetBinContent(b+1));
        stat->SetPointError(np, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinError(b+1));
        sys->SetPointError(np, reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetXaxis()->GetBinWidth(b+1)/2., reference->GetBinContent(b+1) * TMath::Abs(min->GetBinContent(b+1)), reference->GetBinContent(b+1) * TMath::Abs(max->GetBinContent(b+1)));
        np++;
    }
    return {radius, stat, sys};
}

std::set<DataRBin> readDataSpectra(){
    std::set<DataRBin> spectra;
    for(auto r : kJetRadii) {
        std::stringstream filename;
        filename << "data/Systematics1DPt_R" << std::setw(2) << std::setfill('0') << int(r*10) << ".root";
        spectra.insert(readDataSpectrum(filename.str(), r));
    }
    return spectra;
}

TheoryRBin readTheorySpectrum(const std::string_view filename, double r){
    TGraphErrors *spectrum = new TGraphErrors;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto hist = static_cast<TH1 *>(reader->Get("jetptspectrum"));
    int np(0);
    for(auto b : ROOT::TSeqI(0, hist->GetXaxis()->GetNbins())) {
        if(hist->GetXaxis()->GetBinUpEdge(b+1) > 250.) break;
        if(hist->GetXaxis()->GetBinLowEdge(b+1) <30.) continue;
        spectrum->SetPoint(np, hist->GetXaxis()->GetBinCenter(b+1), hist->GetBinContent(b+1));
        spectrum->SetPointError(np, hist->GetXaxis()->GetBinWidth(b+1)/2., hist->GetBinError(b+1));
        np++;
    }
    return {r, spectrum};
}

std::set<TheoryRBin> readTheorySpectra(const std::string_view theory){
    std::set<TheoryRBin> spectra;
    for(auto r : kJetRadii) {
        std::stringstream filename;
        filename << "theory/" << theory << "/spectrum" << theory << "_R" << std::setw(2) << std::setfill('0') << int(r*10.) << ".root";
        spectra.insert(readTheorySpectrum(filename.str(), r));
    }
    return spectra;
}

DataRBin makeRatioDataOverTheory(const DataRBin &data, const TheoryRBin &theory) {
    TGraphErrors *stat = new TGraphErrors;
    TGraphAsymmErrors *sys = new TGraphAsymmErrors;

    auto pointfinder = [](double pt, const TGraphErrors *graph) {
        int result = -1;
        for(auto np : ROOT::TSeqI(0, graph->GetN())){
            if(pt > graph->GetX()[np] - graph->GetEX()[np] && pt < graph->GetX()[np] + graph->GetEX()[np]) {
                result = np;
                break;
            }
        }
        if(result < 0) std::cout << "not found point for pt" << pt << std::endl;
        return result;
    };

    for(auto p : ROOT::TSeqI(0, data.fStat->GetN())){
        int ntheo = pointfinder(data.fStat->GetX()[p], theory.fData);
        if(ntheo < 0) continue;
        auto pointTheory = theory.fData->GetY()[ntheo];
        stat->SetPoint(p, data.fStat->GetX()[p], data.fStat->GetY()[p]/pointTheory);
        stat->SetPointError(p, data.fStat->GetEX()[p], data.fStat->GetEY()[p]/pointTheory);
        sys->SetPoint(p, data.fSys->GetX()[p], data.fSys->GetY()[p]/pointTheory);
        sys->SetPointError(p, data.fSys->GetEXlow()[p], data.fSys->GetEXhigh()[p], data.fSys->GetEYlow()[p]/pointTheory, data.fSys->GetEYhigh()[p]/pointTheory);
    }

    return {data.fRadius, stat, sys};
}

TGraph *makeLineGraph(const TGraphErrors *in) {
    auto result = new TGraph;
    for(auto p : ROOT::TSeqI(0, in->GetN())) result->SetPoint(p, in->GetX()[p], in->GetY()[p]);
    return result;
}

void makeTheoryComparisonSpectrum(){
    auto plot = new ROOT6tools::TSavableCanvas("comparisonSpectraTheory", "Comparison spectra to theory",1000, 1200);
    plot->Divide(2,2);

    auto dataspectra = readDataSpectra();
    auto perugiaspectra = readTheorySpectra("Perugia11");
    auto powhegspectra = readTheorySpectra("POWHEG");

    std::map<double, Color_t> colors = {{0.2, kRed}, {0.3, kGreen+2}, {0.4, kBlue}, {0.5, kViolet}};
    std::map<double, Style_t> markers = {{0.2, 24}, {0.3, 25}, {0.4, 26}, {0.5, 27}};

    for(auto irad : ROOT::TSeqI(0, 4)){
        auto databin =  dataspectra.find({kJetRadii[irad], nullptr, nullptr});
        auto perugiabin = perugiaspectra.find({kJetRadii[irad], nullptr});
        auto powhegbin =  powhegspectra.find({kJetRadii[irad], nullptr});
        plot->cd(irad + 1);
        auto parent = gPad;
        auto specpad = new TPad(Form("specpadR%02d", int(kJetRadii[irad] * 10.)), Form("Spectrum pad R=%.1f", kJetRadii[irad]), 0., 0.35, 1., 1.);
        specpad->Draw();
        specpad->cd();
        specpad->SetLogy();
        specpad->SetBottomMargin(0);
        specpad->SetLeftMargin(0.13);
        auto specframe = new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(kJetRadii[irad] * 10.)), "p_{t} (GeV/c)", "d#sigma/dp_{t}d#eta (mb/(GeV/c))", 0., 250., 5e-8, 1e-1);
        specframe->GetYaxis()->SetLabelSize(0.04);
        specframe->GetYaxis()->SetTitleSize(0.04);
        specframe->GetYaxis()->SetTitleOffset(1.4);
        specframe->Draw("axis");
        TLegend *leg(nullptr);
        if(irad == 0) {
            //(new ROOT6tools::TNDCLabel(0.2, 0.8, 0.89, 0.89, "ALICE preliminary, pp, #sqrt{s} = 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
            (new ROOT6tools::TNDCLabel(0.2, 0.78, 0.55, 0.85, "pp, #sqrt{s} = 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
            (new ROOT6tools::TNDCLabel(0.2, 0.71, 0.4, 0.77, "jets, anti-k_{t}"))->Draw();
            leg = new ROOT6tools::TDefaultLegend(0.55, 0.65, 0.89, 0.79);
            leg->Draw();
        }
        (new ROOT6tools::TNDCLabel(0.2, 0.05, 0.35, 0.12, Form("R=%.1f", kJetRadii[irad])))->Draw();

        Style{colors[kJetRadii[irad]], markers[kJetRadii[irad]]}.SetStyle<TGraphErrors>(*databin->fStat);
        databin->fStat->Draw("epsame");
        if(leg) leg->AddEntry(databin->fStat, "data", "lep");
        databin->fSys->SetFillColor(colors[kJetRadii[irad]]);
        databin->fSys->SetFillStyle(3001);
        databin->fSys->Draw("2same");

        auto perugialine = makeLineGraph(perugiabin->fData);
        perugialine->SetLineColor(kBlack);
        perugialine->SetLineWidth(2);
        perugialine->Draw("lsame");
        if(leg) leg->AddEntry(perugialine, "PYTHIA6, Perugia 2011", "l");

        auto powhegline = makeLineGraph(powhegbin->fData);
        powhegline->SetLineColor(kBlack);
        powhegline->SetLineWidth(2);
        powhegline->SetLineStyle(2);
        powhegline->Draw("lsame");
        if(leg) leg->AddEntry(powhegline, "POWHEG+PYTHIA", "l");

        parent->cd();
        auto ratiopad = new TPad(Form("ratiopadR%02d", int(kJetRadii[irad] * 10.)), Form("Ratio pad R=%.1f", kJetRadii[irad]), 0., 0.0, 1., 0.35);
        ratiopad->Draw();
        ratiopad->cd();
        ratiopad->SetTopMargin(0.);
        ratiopad->SetBottomMargin(0.2);
        ratiopad->SetLeftMargin(0.13);
        auto ratioframe = new ROOT6tools::TAxisFrame(Form("ratframeR%02d", int(kJetRadii[irad] * 10.)), "p_{t} (GeV/c)", "Data / Theory", 0., 250., 0.51, 1.49);
        ratioframe->GetXaxis()->SetTitleSize(0.08);
        ratioframe->GetXaxis()->SetLabelSize(0.08);
        ratioframe->GetYaxis()->SetTitleSize(0.08);
        ratioframe->GetYaxis()->SetLabelSize(0.08);
        ratioframe->GetYaxis()->SetTitleOffset(0.5);
        ratioframe->Draw("axis");
        auto ratio = makeRatioDataOverTheory(*databin, *powhegbin);
        Style{colors[kJetRadii[irad]], markers[kJetRadii[irad]]}.SetStyle<TGraphErrors>(*ratio.fStat);
        ratio.fStat->Draw("epsame");
        ratio.fSys->SetFillColor(colors[kJetRadii[irad]]);
        ratio.fSys->SetFillStyle(3001);
        ratio.fSys->Draw("2same");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}