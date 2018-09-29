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

TGraphErrors *makeRatioTheoryOverData(const DataRBin &data, const TheoryRBin &theory) {
    TGraphErrors *result = new TGraphErrors;

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

    int np(0);
    for(auto p : ROOT::TSeqI(0, theory.fData->GetN())){
        int ndata = pointfinder(theory.fData->GetX()[p], data.fStat);
        if(ndata < 0) continue;
        auto pointData = data.fStat->GetY()[ndata];
        result->SetPoint(np, theory.fData->GetX()[p], theory.fData->GetY()[p]/pointData);
        result->SetPointError(np, theory.fData->GetEX()[p], theory.fData->GetEY()[p]/pointData);
        np++;
    }

    return result;
}

TGraph *makeLineGraph(const TGraphErrors *in) {
    auto result = new TGraph;
    for(auto p : ROOT::TSeqI(0, in->GetN())) result->SetPoint(p, in->GetX()[p], in->GetY()[p]);
    return result;
}

TGraphAsymmErrors *makeErrorRel(TGraphErrors *stat, TGraphAsymmErrors *sys){
    TGraphAsymmErrors *result = new TGraphAsymmErrors;
    for(auto p : ROOT::TSeqI(0, stat->GetN())){
        double elow = TMath::Sqrt(TMath::Power(sys->GetEYlow()[p], 2) + TMath::Power(stat->GetEY()[p], 2)) / stat->GetY()[p],
               ehigh = TMath::Sqrt(TMath::Power(sys->GetEYhigh()[p], 2) + TMath::Power(stat->GetEY()[p], 2)) / stat->GetY()[p];
        result->SetPoint(p, stat->GetX()[p], 1);
        result->SetPointError(p, sys->GetEXlow()[p], sys->GetEXhigh()[p], elow, ehigh);
    }
    return result;
}

void makeTheoryComparisonSpectrum(){
    auto plot = new ROOT6tools::TSavableCanvas("comparisonSpectraTheory", "Comparison spectra to theory",900, 1000);
    plot->Divide(2,2);

    auto dataspectra = readDataSpectra();
    auto perugiaspectra = readTheorySpectra("Perugia11");
    auto powhegspectra = readTheorySpectra("POWHEG");

    std::map<double, Color_t> colors = {{0.2, kRed}, {0.3, kGreen+2}, {0.4, kBlue}, {0.5, kViolet}};
    std::map<double, Style_t> markers = {{0.2, 24}, {0.3, 25}, {0.4, 26}, {0.5, 27}};

    std::map<std::string, Style> theorystyles = {{"pythia6", {kBlue, 24}}, {"powheg", {kRed, 25}}};
    double fracdown  = 0.51;
    std::vector<TPad *> pads = { 
        new TPad("Pad0", "Pad0", 0., fracdown, 0.52, 1.), 
        new TPad("Pad1", "Pad1", 0.52, fracdown, 1., 1.), 
        new TPad("Pad2", "Pad2", 0., 0.0, 0.52, fracdown), 
        new TPad("Pad3", "Pad3", 0.52, 0., 1., fracdown)
    };

    for(auto irad : ROOT::TSeqI(0, 4)){
        auto databin =  dataspectra.find({kJetRadii[irad], nullptr, nullptr});
        auto perugiabin = perugiaspectra.find({kJetRadii[irad], nullptr});
        auto powhegbin =  powhegspectra.find({kJetRadii[irad], nullptr});
        
        plot->cd();
        pads[irad]->Draw();
        pads[irad]->cd();
        auto parent = gPad;
        double splitterSpec = irad < 2 ? 0.27 : 0.35;
        auto specpad = new TPad(Form("specpadR%02d", int(kJetRadii[irad] * 10.)), Form("Spectrum pad R=%.1f", kJetRadii[irad]), 0., splitterSpec, 1., 1.);
        specpad->Draw();
        specpad->cd();
        switch(irad){
            case 0: {
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(0.18);
                gPad->SetTopMargin(0.05);
                break;
            }
            case 1 :{
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(0.11);
                gPad->SetTopMargin(0.05);
                break;
            }
            case 2: {
                gPad->SetTopMargin(0.);
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(0.18);
                break;
            }
            case 3: {
                gPad->SetTopMargin(0.);
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(0.11);
                break;
            }
        };
        specpad->SetLogy();
        specpad->SetBottomMargin(0);
        gPad->SetTicks();
        auto specframe = new ROOT6tools::TAxisFrame(Form("specframeR%02d", int(kJetRadii[irad] * 10.)), "p_{t}^{jet} (GeV/c)", "d#sigma/dp_{t}^{jet}d#eta (mb/(GeV/c))", 0.001, 260., 5e-8, 5e-1);
        auto specysize = irad < 2 ? 0.055 : 0.06;
        auto specyoffset = irad < 2 ? 1.4 : 1.2;
        specframe->GetYaxis()->SetLabelSize(specysize);
        specframe->GetYaxis()->SetTitleSize(specysize);
        specframe->GetYaxis()->SetTitleOffset(specyoffset);
        specframe->Draw("axis");
        TLegend *leg(nullptr);
        if(irad == 0) {
            (new ROOT6tools::TNDCLabel(0.2, 0.8, 0.98, 0.89, "ALICE preliminary, pp, #sqrt{s} = 13 TeV, #intLdt = 4 pb^{-1}"))->Draw();
            //(new ROOT6tools::TNDCLabel(0.2, 0.78, 0.55, 0.85, "pp, #sqrt{s} = 13 TeV, #intLdt = 3.5 pb^{-1}"))->Draw();
            auto jetlabel = new ROOT6tools::TNDCLabel(0.3, 0.55, 0.94, 0.8, "jets, FastJet, anti-k_{t}");
            jetlabel->SetTextAlign(12);
            jetlabel->AddText("p_{t}^{track} > 0.15 GeV/c, E^{cluster} > 0.3 GeV");
            jetlabel->AddText("|#eta^{track}| < 0.7, |#eta^{cluster}| < 0.7, |#eta^{jet}| < 0.7 - R");
            jetlabel->Draw();
        }
        if(irad == 1) {
            leg = new ROOT6tools::TDefaultLegend(0.25, 0.65, 0.89, 0.89);
            leg->Draw();
        }
        (new ROOT6tools::TNDCLabel(gPad->GetLeftMargin() +0.05, gPad->GetBottomMargin()+0.1, gPad->GetLeftMargin()+0.25, gPad->GetBottomMargin() + 0.17, Form("R=%.1f", kJetRadii[irad])))->Draw();

        Style{kBlack, 20}.SetStyle<TGraphErrors>(*databin->fStat);
        databin->fStat->Draw("epsame");
        if(leg) leg->AddEntry(databin->fStat, "data", "lep");
        databin->fSys->SetLineColor(kBlack);
        databin->fSys->SetFillStyle(0);
        databin->fSys->Draw("2same");

        /*
        auto perugialine = makeLineGraph(perugiabin->fData);
        perugialine->SetLineColor(kBlack);
        perugialine->Draw("lsame");
        perugialine->SetLineWidth(2);
        */
        theorystyles["pythia6"].SetStyle<TGraphErrors>(*perugiabin->fData);
        perugiabin->fData->Draw("epsame");
        if(leg) leg->AddEntry(perugiabin->fData, "PYTHIA6, Perugia 2011", "lep");
        //if(leg) leg->AddEntry(perugialine, "PYTHIA6, Perugia 2011", "l");

        /*
        auto powhegline = makeLineGraph(powhegbin->fData);
        powhegline->SetLineColor(kBlack);
        powhegline->SetLineWidth(2);
        powhegline->SetLineStyle(2);
        powhegline->Draw("lsame");
        */
        theorystyles["powheg"].SetStyle<TGraphErrors>(*powhegbin->fData);
        powhegbin->fData->Draw("epsame");
        if(leg) leg->AddEntry(powhegbin->fData, "POWHEG+PYTHIA", "lep");
        //if(leg) leg->AddEntry(powhegline, "POWHEG+PYTHIA", "l");

        parent->cd();
        auto ratiopad = new TPad(Form("ratiopadR%02d", int(kJetRadii[irad] * 10.)), Form("Ratio pad R=%.1f", kJetRadii[irad]), 0., 0.0, 1., splitterSpec);
        ratiopad->Draw();
        ratiopad->cd();
        switch(irad){
            case 0: {
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(0.18);
                gPad->SetBottomMargin(0.);
                break;
            }
            case 1 :{
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(0.11);
                gPad->SetBottomMargin(0.);
                break;
            }
            case 2: {
                gPad->SetTopMargin(0.);
                gPad->SetRightMargin(0.);
                gPad->SetLeftMargin(0.18);
                gPad->SetBottomMargin(0.25);
                break;
            }
            case 3: {
                gPad->SetTopMargin(0.);
                gPad->SetLeftMargin(0.);
                gPad->SetRightMargin(0.11);
                gPad->SetBottomMargin(0.25);
                break;
            }
        };
        ratiopad->SetTopMargin(0.);
        gPad->SetTicks();
        auto ratioframe = new ROOT6tools::TAxisFrame(Form("ratframeR%02d", int(kJetRadii[irad] * 10.)), "p_{t} (GeV/c)", "Theory / Data", 0.001, 260., 0.51, 1.49);
        double ratframesizey = irad < 2 ? 0.13 : 0.105,
               ratframeoffsety = irad < 2 ? 0.6 : 0.7;
        ratioframe->GetXaxis()->SetTitleSize(0.1);
        ratioframe->GetXaxis()->SetLabelSize(0.1);
        ratioframe->GetYaxis()->SetTitleSize(ratframesizey);
        ratioframe->GetYaxis()->SetLabelSize(ratframesizey);
        ratioframe->GetYaxis()->SetTitleOffset(ratframeoffsety);
        ratioframe->Draw("axis");
        auto datarel = makeErrorRel(databin->fStat, databin->fSys);
        datarel->SetFillColor(kGray);
        datarel->SetFillStyle(3001);
        datarel->Draw("2same");
        auto ratioPowheg = makeRatioTheoryOverData(*databin, *powhegbin);
        theorystyles["powheg"].SetStyle<TGraphErrors>(*ratioPowheg);
        ratioPowheg->Draw("epsame");
        auto ratioPythia = makeRatioTheoryOverData(*databin, *perugiabin);
        theorystyles["pythia6"].SetStyle<TGraphErrors>(*ratioPythia);
        ratioPythia->Draw("epsame");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}