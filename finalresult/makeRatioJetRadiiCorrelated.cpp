#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

std::vector<double> kJetRadii = {0.2, 0.3, 0.4, 0.5};

struct DataRBin {
    double              fRadiusNum;
    double              fRadiusDen;
    TGraphErrors        *fStat;
    TGraphAsymmErrors   *fSys;

    bool operator==(const DataRBin &other) const { return TMath::Abs(fRadiusDen - other.fRadiusDen) < DBL_EPSILON; }
    bool operator<(const DataRBin &other) const { return fRadiusDen < other.fRadiusDen; } 
};

struct TheoryRBin {
    double              fRadius;
    TGraphErrors        *fData;

    bool operator==(const TheoryRBin &other) const { return TMath::Abs(fRadius - other.fRadius) < DBL_EPSILON; }
    bool operator<(const TheoryRBin &other) const { return fRadius < other.fRadius; }
};

DataRBin readDataSpectrum(const std::string_view inputfile, double radiusNum, double radiusDen){
    TGraphErrors *stat = new TGraphErrors;
    TGraphAsymmErrors *sys = new TGraphAsymmErrors;

    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    reader->cd();
    std::unique_ptr<TH1> reference(static_cast<TH1 *>(gDirectory->Get("reference")));
    reference->SetDirectory(nullptr);
    gDirectory->cd("sum");
    std::unique_ptr<TH1> min(static_cast<TH1 *>(gDirectory->Get("lowsysrel"))), 
                         max(static_cast<TH1 *>(gDirectory->Get("upsysrel")));
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
    return {radiusNum, radiusDen, stat, sys};
}

std::set<DataRBin> readDataSpectra(){
    std::set<DataRBin> spectra;
    for(auto r : kJetRadii) {
        if(TMath::Abs(r-0.2) < DBL_EPSILON) continue;
        std::stringstream filename;
        filename << "data/Systematics1DPt_R02R" << std::setw(2) << std::setfill('0') << int(r*10) << ".root";
        spectra.insert(readDataSpectrum(filename.str(), 0.2, r));
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

TGraph *ratioTheory(const TheoryRBin &numerator, const TheoryRBin &denominator){
    auto result = new TGraph;
    for(auto p : ROOT::TSeqI(0, numerator.fData->GetN())){
        result->SetPoint(p, numerator.fData->GetX()[p], numerator.fData->GetY()[p]/denominator.fData->GetY()[p]);
    }
    return result;
}

void makeRatioJetRadiiCorrelated(){
    auto spectraData = readDataSpectra();
    auto spectraPerugia = readTheorySpectra("Perugia11"),
         spectraPOWHEG = readTheorySpectra("POWHEG");

    auto plot = new ROOT6tools::TSavableCanvas("ratiojetradii", "Ratio jet radii", 1200, 600);
    plot->Divide(3,1);
    plot->cd(1);
    gPad->SetLeftMargin(0.17);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.13);
    gPad->SetTopMargin(0.05);
    auto frameR0302 = new ROOT6tools::TAxisFrame("ratioframeR0302", "p_{t}^{jet} (GeV/c)", "(d#sigma(R=0.2)/dp_{t}^{jet}d#eta)/(d#sigma(R=0.3)/dp_{t}^{jet}d#eta)", 0., 250., 0.0, 1.4);
    frameR0302->GetXaxis()->SetLabelSize(0.05);
    frameR0302->GetXaxis()->SetTitleSize(0.05);
    frameR0302->GetYaxis()->SetLabelSize(0.05);
    frameR0302->GetYaxis()->SetTitleSize(0.05);
    frameR0302->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.2, 0.15, 0.89, 0.4);
    leg->SetTextSize(0.05);
    leg->Draw();
    auto prelimlabel = new ROOT6tools::TNDCLabel(0.2, 0.61, 0.94, 0.94, "ALICE preliminary ");
    prelimlabel->SetTextAlign(12);
    prelimlabel->AddText("pp, #sqrt{s} = 13 TeV, #intLdt = 4 pb^{-1}");
    prelimlabel->AddText("Jets, FastJet, anti-k_{t}");
    prelimlabel->AddText("p_{t}^{ch} > 0.15 GeV/c, E^{cluster} = 0.3 GeV");
    prelimlabel->AddText("|#eta^{jet}| < 0.7 - R");
    prelimlabel->Draw();
    auto dataR0302 = *spectraData.find({0.2, 0.3, nullptr, nullptr});
    Style{kBlack, 24}.SetStyle<TGraphErrors>(*dataR0302.fStat);
    dataR0302.fStat->Draw("epsame");
    leg->AddEntry(dataR0302.fStat, "Data", "lep");
    dataR0302.fSys->SetFillColor(kBlack);
    dataR0302.fSys->SetFillStyle(0);
    dataR0302.fSys->Draw("2same");
    auto perugiaR03R02 = ratioTheory(*spectraPerugia.find({0.2, nullptr}), *spectraPerugia.find({0.3, nullptr})),
         powhegR03R02 = ratioTheory(*spectraPOWHEG.find({0.2, nullptr}), *spectraPOWHEG.find({0.3, nullptr}));
    perugiaR03R02->SetLineColor(kRed);
    perugiaR03R02->SetLineStyle(3);
    perugiaR03R02->SetLineWidth(2);
    perugiaR03R02->Draw("lsame");
    powhegR03R02->SetLineColor(kBlue);
    powhegR03R02->SetLineWidth(2);
    powhegR03R02->SetLineStyle(2);
    powhegR03R02->Draw("lsame");
    leg->AddEntry(perugiaR03R02, "PYTHIA6 Perugia2011", "l");
    leg->AddEntry(powhegR03R02, "PYTHIA + POWHEG", "l");

    plot->cd(2);
    gPad->SetLeftMargin(0.17);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.13);
    gPad->SetTopMargin(0.05);
    auto frameR0402 = new ROOT6tools::TAxisFrame("ratioframeR0402", "p_{t} (GeV/c)", "(d#sigma(R=0.2)/dp_{t}^{jet}d#eta)/(d#sigma(R=0.4)/dp_{t}^{jet}d#eta)", 0., 250., 0.0, 1.4);
    frameR0402->GetXaxis()->SetLabelSize(0.05);
    frameR0402->GetXaxis()->SetTitleSize(0.05);
    frameR0402->GetYaxis()->SetLabelSize(0.05);
    frameR0402->GetYaxis()->SetTitleSize(0.05);
    frameR0402->Draw("axis");
    auto dataR0402 = *spectraData.find({0.2, 0.4, nullptr, nullptr});
    Style{kBlack, 24}.SetStyle<TGraphErrors>(*dataR0402.fStat);
    dataR0402.fStat->Draw("epsame");
    dataR0402.fSys->SetLineColor(kBlack);
    dataR0402.fSys->SetFillStyle(0);
    dataR0402.fSys->Draw("2same");
    auto perugiaR04R02 = ratioTheory(*spectraPerugia.find({0.2, nullptr}), *spectraPerugia.find({0.4, nullptr})),
         powhegR04R02 = ratioTheory(*spectraPOWHEG.find({0.2, nullptr}), *spectraPOWHEG.find({0.4, nullptr}));
    perugiaR04R02->SetLineColor(kRed);
    perugiaR04R02->SetLineStyle(3);
    perugiaR04R02->SetLineWidth(2);
    perugiaR04R02->Draw("lsame");
    powhegR04R02->SetLineColor(kBlue);
    powhegR04R02->SetLineWidth(2);
    powhegR04R02->SetLineStyle(2);
    powhegR04R02->Draw("lsame");

    plot->cd(3);
    gPad->SetLeftMargin(0.17);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.13);
    gPad->SetTopMargin(0.05);
    auto frameR0502 = new ROOT6tools::TAxisFrame("ratioframeR0402", "p_{t} (GeV/c)", "(d#sigma(R=0.2)/dp_{t}^{jet}d#eta)/(d#sigma(R=0.5)/dp_{t}^{jet}d#eta)", 0., 250., 0.0, 1.4);
    frameR0502->GetXaxis()->SetLabelSize(0.05);
    frameR0502->GetXaxis()->SetTitleSize(0.05);
    frameR0502->GetYaxis()->SetLabelSize(0.05);
    frameR0502->GetYaxis()->SetTitleSize(0.05);
    frameR0502->Draw("axis");
    auto dataR0502 = *spectraData.find({0.2, 0.5, nullptr, nullptr});
    Style{kBlack, 24}.SetStyle<TGraphErrors>(*dataR0502.fStat);
    dataR0502.fStat->Draw("epsame");
    dataR0502.fSys->SetLineColor(kBlack);
    dataR0502.fSys->SetFillStyle(0);
    dataR0502.fSys->Draw("2same");
    auto perugiaR05R02 = ratioTheory(*spectraPerugia.find({0.2, nullptr}), *spectraPerugia.find({0.5, nullptr})),
         powhegR05R02 = ratioTheory(*spectraPOWHEG.find({0.2, nullptr}), *spectraPOWHEG.find({0.5, nullptr}));
    perugiaR05R02->SetLineColor(kRed);
    perugiaR05R02->SetLineStyle(3);
    perugiaR05R02->SetLineWidth(2);
    perugiaR05R02->Draw("lsame");
    powhegR05R02->SetLineColor(kBlue);
    powhegR05R02->SetLineWidth(2);
    powhegR05R02->SetLineStyle(2);
    powhegR05R02->Draw("lsame");

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}