#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/filesystem.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"
#include "../../struct/JetSpectrumReader.cxx"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/Ratio.cxx"

struct outputdata{
    double              fRDenom;
    TH1*                fDefaultSpectrum;
    TH1*                fVarSpectrum;
    TH1*                fBarlow;
    TH1 *               fAbsDiff;
    Ratio*              fRatio;

    bool operator<(const outputdata &other) const { return fRDenom < other.fRDenom; }
    bool operator==(const outputdata &other) const { return TMath::Abs(fRDenom - other.fRDenom) < 1e-6; }
};

std::map<int, TH1 *> getCorrected(const std::string_view filename, const std::string_view varname, int regularization  = 4) {
    std::vector<std::string> spectra = {Form("normalized_reg%d", regularization)};
    std::map<int, TH1 *> result;
    JetSpectrumReader reader(filename, spectra);
    for(auto radius : reader.GetJetSpectra().GetJetRadii()) {
        auto spec = reader.GetJetSpectrum(radius, spectra[0]);  
        spec->SetNameTitle(Form("Spec%s_R%02d", varname.data(), int(radius*10.)), varname.data());
        spec->SetDirectory(nullptr);
        result[int(radius*10)] = spec;
    }
    return result;
}

TH1 *makeBarlow(const TH1 *variation, const TH1 *defaultdist){
    auto barlow = histcopy(defaultdist);
    barlow->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, defaultdist->GetXaxis()->GetNbins())) {
        auto diff = TMath::Abs(defaultdist->GetBinContent(b+1) - variation->GetBinContent(b+1)),
             sigma = TMath::Sqrt(TMath::Abs(TMath::Power(defaultdist->GetBinError(b+1), 2) - TMath::Power(variation->GetBinError(b+1), 2)));
        barlow->SetBinContent(b+1, diff/sigma);
        barlow->SetBinError(b+1, 0);
    }
    return barlow;
}

TH1 *makeRatio(const TH1 *variation, const TH1 *defaultdist){
    auto ratio = histcopy(variation);
    ratio->SetDirectory(nullptr);
    ratio->Divide(defaultdist);
    return ratio;
}

void testVariationRatio1D(const std::string_view varname, 
                          const std::string_view vartitle, 
                          const std::string_view defaultfile, 
                          const std::string_view varfile, 
                          int regdefault = 4, int regvar = 4){
    auto specDefault = getCorrected(defaultfile, "default", regdefault), specVariation = getCorrected(varfile, varname, regvar); 
    Style defaultstyle{kRed, 24}, varstyle{kBlue, 25};
    gROOT->cd();

    TH1 *numeratorDefault = specDefault.find(2)->second, *numeratorVariation = specVariation.find(2)->second;
    std::map<int, TH1 *> ratiosDefault, ratiosVar;
    for(auto irad : ROOT::TSeqI(3, 7)) {
        auto ratiodefault = (TH1 *)numeratorDefault->Clone("DefaultRatio"), ratiovar = (TH1 *)numeratorVariation->Clone("VariationRatio");
        ratiodefault->SetDirectory(nullptr);
        ratiovar->SetDirectory(nullptr);
        ratiodefault->Divide(numeratorDefault, specDefault.find(irad)->second, 1., 1., "b");
        ratiovar->Divide(numeratorDefault, specVariation.find(irad)->second, 1., 1.,  "b");
        ratiosDefault[irad] = ratiodefault;
        ratiosVar[irad] = ratiovar;
    } 
    
    std::set<outputdata> output;
    double ptmax = 350.;
    for(auto r : ROOT::TSeqI(3, 7)){
        auto compplot = new ROOT6tools::TSavableCanvas(Form("RatioR02R%02d_%s", r, varname.data()), Form("Comparison %s, ratio R=0.2/R=%.1f", varname.data(), double(r)/10.), 1200, 1000);
        compplot->Divide(3,1);

        compplot->cd(1);
        GraphicsPad specpad(gPad);
        specpad.Frame(Form("compframe_R02R%02d", r), "p_{t} (GeV/c)", Form("R=0.2/R=%.1f", double(r)/20.), 0., ptmax, 0., 1.);
        specpad.Legend(0.15, 0.7, 0.89, 0.89);

        auto defaultspec = ratiosDefault.find(r)->second, varspec = ratiosVar.find(r)->second;
        specpad.Draw<TH1>(defaultspec, defaultstyle, "Default");
        specpad.Draw<TH1>(varspec, varstyle, vartitle);

        compplot->cd(2);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("ratioframe_R02R%02d", r), "p_{t} (GeV/c)", "variation / default", 0., ptmax, 0.5, 1.5);
        Ratio *varratio = new Ratio(varspec, defaultspec);
        ratiopad.Draw<Ratio>(varratio, defaultstyle);

        compplot->cd(3);
        GraphicsPad barlowpad(gPad);
        barlowpad.Frame(Form("barlowframe_R02R%02d", r), "p_{t} (GeV/c)", "(default-variation)/(#sqrt{|#sigma_{var}^{2} - sigma_{default}^{2}|})", 0., ptmax, 0., 10.);
        auto barlow = makeBarlow(varspec, defaultspec);
        barlow->SetNameTitle("barlowtest", "Barlow test");
        barlow->SetLineColor(kBlack);
        barlow->SetFillColor(kRed);
        barlow->Draw("boxsame");

        compplot->cd();
        compplot->Update();
        compplot->SaveCanvas(compplot->GetName());

        // Calculate absolute difference
        auto absdiff = histcopy(defaultspec);
        absdiff->SetNameTitle("absdiff", "Absolute difference");
        for(auto b : ROOT::TSeqI(0, absdiff->GetXaxis()->GetNbins())){
            absdiff->SetBinContent(b+1, TMath::Abs(varspec->GetBinContent(b+1) - defaultspec->GetBinContent(b+1))/ defaultspec->GetBinContent(b+1));
            absdiff->SetBinError(b+1, 0.);
        }

        output.insert({double(r)/10., defaultspec, varspec, barlow, absdiff, varratio}); 
    }
    
    // Create output rootfile
    std::unique_ptr<TFile> outputwriter(TFile::Open(Form("ratiosSystematics_%s.root", varname.data()), "RECREATE"));
    outputwriter->cd();
    for(const auto &rbin : output){
        std::string dirname(Form("R02R%02d", int(rbin.fRDenom * 10.)));
        outputwriter->mkdir(dirname.data());
        outputwriter->cd(dirname.data());
        rbin.fDefaultSpectrum->Write("DefaultRatio");
        rbin.fVarSpectrum->Write("VariationRatio");
        rbin.fRatio->makeTH1("ratioDefaultVar", "Ratio default variation")->Write("RatioCRS");
        rbin.fBarlow->Write("barlowtest");
        rbin.fAbsDiff->Write("absdiff");
    }
}