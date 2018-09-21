#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

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

std::pair<TGraphErrors *, TGraphAsymmErrors *> ratioData(const DataRBin &numerator, const DataRBin &denominator) {
    TGraphErrors *stat = new TGraphErrors;
    TGraphAsymmErrors *sys = new TGraphAsymmErrors;

    for(auto p : ROOT::TSeqI(0, numerator.fStat->GetN())) {
        stat->SetPoint(p, numerator.fStat->GetX()[p], numerator.fStat->GetY()[p]/denominator.fStat->GetY()[p]);
        sys->SetPoint(p, numerator.fSys->GetX()[p], numerator.fSys->GetY()[p]/denominator.fSys->GetY()[p]);
        stat->SetPointError(p, 
                            numerator.fStat->GetEX()[p], 
                            numerator.fStat->GetY()[p]/denominator.fStat->GetY()[p] * TMath::Sqrt(TMath::Power(numerator.fStat->GetEY()[p]/numerator.fStat->GetY()[p], 2) + TMath::Power(denominator.fStat->GetEY()[p]/denominator.fStat->GetY()[p], 2))
                            );
        sys->SetPointError(p, 
                            numerator.fSys->GetEXlow()[p], 
                            numerator.fSys->GetEXhigh()[p], 
                            numerator.fSys->GetY()[p]/denominator.fSys->GetY()[p] * TMath::Sqrt(TMath::Power(numerator.fSys->GetEYlow()[p]/numerator.fSys->GetY()[p], 2) + TMath::Power(denominator.fSys->GetEYhigh()[p]/denominator.fSys->GetY()[p], 2)),
                            numerator.fSys->GetY()[p]/denominator.fSys->GetY()[p] * TMath::Sqrt(TMath::Power(numerator.fSys->GetEYhigh()[p]/numerator.fSys->GetY()[p], 2) + TMath::Power(denominator.fSys->GetEYlow()[p]/denominator.fSys->GetY()[p], 2))
                            );
    }

    return {stat, sys};
}

TGraph *ratioTheory(const TheoryRBin &numerator, const TheoryRBin &denominator){
    auto result = new TGraph;
    for(auto p : ROOT::TSeqI(0, numerator.fData->GetN())){
        result->SetPoint(p, numerator.fData->GetX()[p], numerator.fData->GetY()[p]/denominator.fData->GetY()[p]);
    }
    return result;
}

void makeRatioJetRadii(){
    auto spectraData = readDataSpectra();
    auto spectraPerugia = readTheorySpectra("Perugia11"),
         spectraPOWHEG = readTheorySpectra("POWHEG");

    auto plot = new ROOT6tools::TSavableCanvas("ratiojetradii", "Ratio jet radii", 1200, 600);
    plot->Divide(3,1);
    plot->cd(1);
    (new ROOT6tools::TAxisFrame("ratioframeR0302", "p_{t} (GeV/c)", "(d#sigma(R=0.2)/dp_{t}d#eta)/(d#sigma(R=0.3)/dp_{t}d#eta)", 0., 250., 0.0, 1.2))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.45, 0.15, 0.89, 0.3);
    leg->Draw();
    (new ROOT6tools::TNDCLabel(0.3, 0.81, 0.89, 0.89, "pp, #sqrt{s} = 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
    (new ROOT6tools::TNDCLabel(0.6, 0.74, 0.89, 0.8, "jets, anti-k_{t}"))->Draw();
    auto dataR0302 = ratioData(*spectraData.find({0.2, nullptr, nullptr}),*spectraData.find({0.3, nullptr, nullptr}));
    Style{kRed, 24}.SetStyle<TGraphErrors>(*dataR0302.first);
    dataR0302.first->Draw("epsame");
    leg->AddEntry(dataR0302.first, "Data", "lep");
    dataR0302.second->SetFillColor(kRed);
    dataR0302.second->SetFillStyle(3001);
    dataR0302.second->Draw("2same");
    auto perugiaR03R02 = ratioTheory(*spectraPerugia.find({0.2, nullptr}), *spectraPerugia.find({0.3, nullptr})),
         powhegR03R02 = ratioTheory(*spectraPOWHEG.find({0.2, nullptr}), *spectraPOWHEG.find({0.3, nullptr}));
    perugiaR03R02->SetLineColor(kRed);
    perugiaR03R02->SetLineWidth(2);
    perugiaR03R02->Draw("lsame");
    powhegR03R02->SetLineColor(kRed);
    powhegR03R02->SetLineWidth(2);
    powhegR03R02->SetLineStyle(2);
    powhegR03R02->Draw("lsame");
    leg->AddEntry(perugiaR03R02, "PYTHIA6 Perugia2011", "l");
    leg->AddEntry(powhegR03R02, "PYTHIA + POWHEG", "l");

    plot->cd(2);
    (new ROOT6tools::TAxisFrame("ratioframeR0402", "p_{t} (GeV/c)", "(d#sigma(R=0.2)/dp_{t}d#eta)/(d#sigma(R=0.4)/dp_{t}d#eta)", 0., 250., 0.0, 1.2))->Draw("axis");
    auto dataR0402 = ratioData(*spectraData.find({0.2, nullptr, nullptr}),*spectraData.find({0.4, nullptr, nullptr}));
    Style{kBlue, 25}.SetStyle<TGraphErrors>(*dataR0402.first);
    dataR0402.first->Draw("epsame");
    dataR0402.second->SetFillColor(kBlue);
    dataR0402.second->SetFillStyle(3001);
    dataR0402.second->Draw("2same");
    auto perugiaR04R02 = ratioTheory(*spectraPerugia.find({0.2, nullptr}), *spectraPerugia.find({0.4, nullptr})),
         powhegR04R02 = ratioTheory(*spectraPOWHEG.find({0.2, nullptr}), *spectraPOWHEG.find({0.4, nullptr}));
    perugiaR04R02->SetLineColor(kBlue);
    perugiaR04R02->SetLineWidth(2);
    perugiaR04R02->Draw("lsame");
    powhegR04R02->SetLineColor(kBlue);
    powhegR04R02->SetLineWidth(2);
    powhegR04R02->SetLineStyle(2);
    powhegR04R02->Draw("lsame");

    plot->cd(3);
    (new ROOT6tools::TAxisFrame("ratioframeR0402", "p_{t} (GeV/c)", "(d#sigma(R=0.2)/dp_{t}d#eta)/(d#sigma(R=0.5)/dp_{t}d#eta)", 0., 250., 0.0, 1.2))->Draw("axis");
    auto dataR0502 = ratioData(*spectraData.find({0.2, nullptr, nullptr}),*spectraData.find({0.5, nullptr, nullptr}));
    Style{kGreen+2, 26}.SetStyle<TGraphErrors>(*dataR0502.first);
    dataR0502.first->Draw("epsame");
    dataR0502.second->SetFillColor(kGreen+2);
    dataR0502.second->SetFillStyle(3001);
    dataR0502.second->Draw("2same");
    auto perugiaR05R02 = ratioTheory(*spectraPerugia.find({0.2, nullptr}), *spectraPerugia.find({0.5, nullptr})),
         powhegR05R02 = ratioTheory(*spectraPOWHEG.find({0.2, nullptr}), *spectraPOWHEG.find({0.5, nullptr}));
    perugiaR05R02->SetLineColor(kGreen+2);
    perugiaR05R02->SetLineWidth(2);
    perugiaR05R02->Draw("lsame");
    powhegR05R02->SetLineColor(kGreen+2);
    powhegR05R02->SetLineWidth(2);
    powhegR05R02->SetLineStyle(2);
    powhegR05R02->Draw("lsame");

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}