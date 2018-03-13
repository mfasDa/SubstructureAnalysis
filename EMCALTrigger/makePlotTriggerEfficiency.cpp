#ifndef __CLING__
#include <algorithm>
#include <array>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TString.h>
#endif

const std::vector<std::string> kEMCALtriggers = {"EJ1", "EJ2"};
const std::map<std::string, Color_t> colors = {{"INT7", kBlack}, {"EJ1", kRed}, {"EJ2", kBlue}};
const std::map<std::string, Style_t> markers = {{"INT7", 20}, {"EJ1", 24}, {"EJ2", 25}};

struct Spectra{
    double fR;
    std::vector<TH1 *> fSpectra;
};

std::vector<std::string> tokenize(const std::string &str, char token){
    std::stringstream parser(str);
    std::string tmp; 
    std::vector<std::string> out;
    while(std::getline(parser, tmp, token)) out.emplace_back(tmp);
    return out;
}

std::vector<Spectra> ExtractSpectraFromFile(const std::string_view filename, const std::vector<std::string> &triggers){
    std::vector<Spectra> result;
    auto reader = std::unique_ptr<TFile>(TFile::Open(filename.data(), "READ"));
    std::cout << "Extracting from file " << filename <<std::endl;

    std::string jettype = std::string(filename.substr(filename.find_last_of("_") + 1, filename.find(".") - filename.find_last_of("_") - 1));
    std::cout << "Jet type: " << jettype << std::endl;

    std::vector<double> jetradii;
    for(auto k : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
        TString keyname = k->GetName();
        auto index = keyname.Index("R");
        if(index) {
            auto radius = double(TString(keyname(index+1, 2)).Atoi())/10.;
            if(std::find(jetradii.begin(), jetradii.end(), radius) == jetradii.end()) jetradii.emplace_back(radius);
        }
    }
    std::sort(jetradii.begin(), jetradii.end(), std::less<double>());

    std::cout << "Found jet radii:" << std::endl;
    std::cout << "==============================" << std::endl;
    for(auto r : jetradii) {
        std::cout << r << std::endl;
    }

    for(auto r : jetradii) {
        std::vector<TH1 *>hists;
        for(auto t : triggers) {
            std::cout << "Getting trigger " << t << std::endl;
            auto h = static_cast<TH1 *>(reader->Get(Form("hPtJet%s_R%02d_%s", jettype.data(), int(r * 10.), t.data())));
            h->SetDirectory(nullptr);
            h->SetName(t.data());
            hists.emplace_back(h);
        }
        result.push_back({r, hists});
    }
    return result;

}

std::vector<TH1 *> ExtractTriggerEfficiencies(const Spectra &triggers, const Spectra &minbias){
    std::vector<TH1 *>efficiencies;
    TH1 *mbref = *(std::find_if(minbias.fSpectra.begin(), minbias.fSpectra.end(), [](const TH1 *hist) -> bool { return TString(hist->GetName()) == "INT7"; }));
    for(auto t : triggers.fSpectra) {
        auto eff = static_cast<TH1 *>(t->Clone(Form("efficiency_R%02d_%s", int(triggers.fR*10), t->GetName())));
        eff->SetDirectory(nullptr);
        eff->Divide(eff, mbref, 1., 1., "b");
        efficiencies.emplace_back(eff);
    }
    return efficiencies;
}

std::vector<Spectra> ExtractTriggeredSpectra(const std::string_view jettype, bool wrejection, int version){
    std::stringstream filename;
    filename << "merge_trigger/normalized_" << jettype;
    if(wrejection){
        filename << "_wrejection";
	if(version == 1)
	    filename << "_v1";
    }
    filename << ".root";
    return ExtractSpectraFromFile(filename.str(), kEMCALtriggers);
}

std::vector<Spectra> ExtractMinBiasSpectra(const std::string_view jettype, bool wrejection, int version){
    std::vector<std::string> mbtriggers = {"INT7"};
    std::stringstream filename;
    filename << "merge_minbias/normalized_" << jettype;
    if(wrejection){
        filename << "_wrejection";
	if(version == 1)
	    filename << "_v1";
    }
    filename << ".root";
    return ExtractSpectraFromFile(filename.str(), mbtriggers);
}

void PlotSpectra(const Spectra &triggers, const Spectra &mb, bool doleg){
    gPad->SetLogy();
    auto frame = new TH1F(Form("FrameR%02d", int(triggers.fR * 10.)), "; p_{t,j} (GeV/c); 1/N_{ev} dN/dp_{t} ((GeV/c)^{-1})", 200., 0., 200);
    frame->SetDirectory(nullptr);
    frame->SetStats(false);
    frame->GetYaxis()->SetRangeUser(1e-9, 1e-3);
    frame->Draw("axis");

    TLegend *leg = nullptr;
    if(doleg){
        leg = new TLegend(0.65, 0.7, 0.89, 0.89);
        leg->SetBorderSize(0);
        leg->SetFillStyle(0);
        leg->SetTextFont(42);
        leg->Draw();
    }
    
    auto label = new TPaveText(0.15, 0.15, 0.45, 0.22, "NDC");
    label->SetBorderSize(0);
    label->SetFillStyle(0);
    label->SetTextFont(42);
    label->AddText(Form("Full jets, R=%0.1f", triggers.fR));
    label->Draw();

    auto mbtrigger = mb.fSpectra[0];
    mbtrigger->SetMarkerColor(colors.find("INT7")->second);
    mbtrigger->SetLineColor(colors.find("INT7")->second);
    mbtrigger->SetMarkerStyle(markers.find("INT7")->second);
    mbtrigger->Draw("epsame");
    if(leg) leg->AddEntry(mbtrigger, "INT7", "lep");

    for(auto t : triggers.fSpectra) {
        t->SetMarkerColor(colors.find(t->GetName())->second);
        t->SetLineColor(colors.find(t->GetName())->second);
        t->SetMarkerStyle(markers.find(t->GetName())->second);
        t->Draw("epsame");
        if(leg) leg->AddEntry(t, t->GetName(), "lep");
    }
    gPad->Update();
}

void PlotEfficiency(std::vector<TH1 *> efficiencies, double r){
   auto frame = new TH1F(Form("effframeR%02d", int(r*10.)), "; p_{t,j} (GeV/c); Trigger efficiency", 200, 0., 200.);
   frame->SetDirectory(nullptr);
   frame->SetStats(false);
   frame->GetYaxis()->SetRangeUser(0., 1.5);
   frame->Draw("axis");

   for(auto e : efficiencies){
       auto tokens = tokenize(e->GetName(), '_');
       const auto &trigger = tokens[2];
       e->SetMarkerColor(colors.find(trigger)->second);
       e->SetLineColor(colors.find(trigger)->second);
       e->SetMarkerStyle(markers.find(trigger)->second);
       e->Draw("epsame");
   }
   gPad->Update();
}

void makePlotTriggerEfficiency(const std::string_view jettype, bool wrejection, int version = 0){
    auto spectraTrigger = ExtractTriggeredSpectra(jettype, wrejection, version), spectraMB = ExtractMinBiasSpectra(jettype, wrejection, version);
    std::vector<double> jetradii;
    std::for_each(spectraTrigger.begin(), spectraTrigger.end(), [&jetradii](const Spectra &spec) { if(std::find(jetradii.begin(), jetradii.end(), spec.fR) == jetradii.end()) jetradii.push_back(spec.fR); });
    std::sort(jetradii.begin(), jetradii.end(), std::less<double>());

    auto plot = new TCanvas("triggerEfficiency", "Trigger efficiency comparison", 500 * jetradii.size(), 800);
    plot->Divide(jetradii.size(),2);

    auto irad = 0;
    std::vector<TH1 *> efficienciesAll;
    for(auto r : jetradii){
        plot->cd(irad+1);
        auto rmatcher = [&r](const Spectra & s) -> bool { return TMath::Abs(s.fR - r ) < 1e-5; };
        auto rtriggers = *(std::find_if(spectraTrigger.begin(), spectraTrigger.end(), rmatcher));
        auto rmb = *(std::find_if(spectraMB.begin(), spectraMB.end(), rmatcher));
        PlotSpectra(rtriggers, rmb, irad == 0);

        plot->cd(irad + jetradii.size() + 1);
        auto refficiencies = ExtractTriggerEfficiencies(rtriggers, rmb);
        for(auto eff : refficiencies) efficienciesAll.emplace_back(eff);
        PlotEfficiency(refficiencies, r);
        irad++;
    }
    plot->cd();
    plot->Update();

    std::stringstream filename;
    filename << "efficiencies";
    if(wrejection) filename << "_wrejection";
    filename << ".root";
    auto writer = std::unique_ptr<TFile>(TFile::Open(filename.str().data(), "RECREATE"));
    writer->cd();
    for(auto e : efficienciesAll) e->Write();
}
