#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/root.C"

TH1 *truncate(const TH1 *input, double ptmin, double ptmax) {
    std::vector<double> bins;
    for(auto b : ROOT::TSeqI(0, input->GetXaxis()->GetNbins())) {
        if(input->GetXaxis()->GetBinLowEdge(b+1) < ptmin) continue;
        if(input->GetXaxis()->GetBinLowEdge(b+1) > ptmax) break;
        bins.emplace_back(input->GetXaxis()->GetBinLowEdge(b+1));
    }
    if(input->GetXaxis()->GetBinUpEdge(input->GetXaxis()->GetNbins()) <= ptmax) bins.emplace_back(input->GetXaxis()->GetBinUpEdge(input->GetXaxis()->GetNbins()));
    auto result = new TH1D(Form("%struncated", input->GetName()), input->GetTitle(), bins.size()-1, bins.data());
    result->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
        auto ib = input->GetXaxis()->FindBin(result->GetXaxis()->GetBinCenter(b+1));
        result->SetBinContent(b+1, input->GetBinContent(ib));
        result->SetBinError(b+1, input->GetBinError(ib));
    }
    return result;
}

TH1 *readDataSpectrum(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("iteration4");
    std::unique_ptr<TH1> spectrum(static_cast<TH1 *>(gDirectory->Get("normalized_iter4")));
    spectrum->SetDirectory(nullptr);
    return truncate(spectrum.get(), 30, 240);
}

TH1 *readTheorySpectrum(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::unique_ptr<TH1> spec(static_cast<TH1 *>(reader->Get("jetptspectrum")));
    spec->SetDirectory(nullptr);
    return truncate(spec.get(), 30, 240);
}

void makeComparisonCorrectionMethods(){
    auto plot = new ROOT6tools::TSavableCanvas("methodcomp1D", "method comparison 1D", 1400, 800);
    plot->Divide(4,2);
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    std::map<std::string, Style> styles = {{"combined", {kBlack, 20}}, {"newNonLinCorr", {kViolet, 28}}, {"onlyINT7", {kRed, 24}}, {"onlyEJ1", {kBlue, 25}}, {"POWHEG", {kViolet, 26}}};
    std::vector<std::string> datacomb = {"combined", "newNonLinCorr", "onlyINT7", "onlyEJ1"}, theorycomp = {"POWHEG"};
    
    for(auto irad : ROOT::TSeqI(0, 4)){
        plot->cd(irad+1);
        gPad->SetLogy();
        gPad->SetLeftMargin(0.14);
        gPad->SetRightMargin(0.04);
        (new ROOT6tools::TAxisFrame(Form("specframe%d", irad), "p_{t} (GeV)", "d#sigma/dp_{t}d#eta (mb/GeV/c^{-1})",0., 250., 1e-10, 10))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.4, 0.22, Form("R=%.1f", jetradii[irad])))->Draw();
        TLegend *leg = nullptr;
        if(!irad){
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.6, 0.89, 0.89);
            leg->Draw();
        }
        std::vector<TH1 *> spectra;
        TH1 *reference;
        for(auto s : datacomb) {
            auto spec = readDataSpectrum(Form("%s/corrected1DBayes_R%02d.root", s.data(), int(jetradii[irad]*10.)));
            styles[s].SetStyle<TH1>(*spec);
            spec->Draw("epsame");
            if(leg) leg->AddEntry(spec, s.data(), "lep");
            if(s == "combined") reference = spec;
            else spectra.push_back(spec);
        }
        for(auto t : theorycomp) {
            auto spec = readTheorySpectrum(Form("%s/spectrum%s_R%02d.root", t.data(), t.data(), int(jetradii[irad]*10.)));
            styles[t].SetStyle<TH1>(*spec);
            spec->Draw("epsame");
            if(leg) leg->AddEntry(spec, t.data(), "lep");
            spectra.push_back(spec);
        }

        plot->cd(irad+5);
        (new ROOT6tools::TAxisFrame(Form("ratioframe%d", irad), "p_{t} (GeV)", "ratio to combined",0., 250., 0., 2.))->Draw("axis");
        for(auto s : spectra) {
            auto ratio = histcopy(s);
            ratio->SetDirectory(nullptr);
            ratio->Divide(reference);
            ratio->Draw("epsame");
        }
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}