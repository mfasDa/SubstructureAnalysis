
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
    double              fR;
    TH1*                fDefaultSpectrum;
    TH1*                fVarSpectrum;
    TH1*                fBarlow;
    TH1 *               fAbsDiff;
    Ratio*              fRatio;

    bool operator<(const outputdata &other) const { return fR < other.fR; }
    bool operator==(const outputdata &other) const { return TMath::Abs(fR - other.fR) < 1e-6; }
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

void testVariation1D(const std::string_view varname, const std::string_view vartitle, const std::string_view defaultfile, const std::string_view varfile, int regularizationDefault = 4, int regularizationVar = 4){
    auto specDefault = getCorrected(defaultfile, "default", regularizationDefault), specVariation = getCorrected(varfile, varname, regularizationVar);
    Style defaultstyle{kRed, 24}, varstyle{kBlue, 25};
    gROOT->cd();

    std::set<outputdata> output;
    double ptmax = 350.;
    for(double radius = 0.2; radius <= 0.6; radius += 0.1){
        //if(TMath::Abs(radius-0.3) < 1e-5) continue;
        printf("Doing r=%.1f\n", radius);
        std::string rstring(Form("R%02d", int(radius*10.)));
        auto compplot = new ROOT6tools::TSavableCanvas(Form("comp_%s_%s", varname.data(), rstring.data()), Form("Comparison %s, R = %.1f", varname.data(), radius), 1200, 1000);
        compplot->Divide(3,1);

        compplot->cd(1);
        GraphicsPad specpad(gPad);
        specpad.Logy();
        specpad.Frame(Form("compframe_%s", rstring.data()), "p_{t} (GeV/c)", "dsigma/(dp_{t} d#eta) (mb/(GeV/c))", 0., ptmax, 1e-10, 1e-1);
        specpad.Label(0.15, 0.15, 0.45, 0.21, Form("jets, R=%.1f", radius));
        specpad.Legend(0.55, 0.7, 0.89, 0.89);

        auto foundDefault = specDefault.find(int(radius*10.)),
             foundVariation = specVariation.find(int(radius*10.)); 
        if(foundDefault == specDefault.end()) std::cout << "default " << radius << " not found" << std::endl;
        if(foundVariation == specVariation.end()) std::cout << "variation " << radius << " not found" << std::endl;
        auto defaultspec = foundDefault->second,
             varspec = foundVariation->second;
        specpad.Draw<TH1>(defaultspec, defaultstyle, "Default");
        specpad.Draw<TH1>(varspec, varstyle, vartitle);

        compplot->cd(2);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("ratioframe_%s", rstring.data()), "p_{t} (GeV/c)", "variation / default", 0., ptmax, 0.5, 1.5);
        Ratio *varratio = new Ratio(varspec, defaultspec);
        ratiopad.Draw<Ratio>(varratio, defaultstyle);

        compplot->cd(3);
        GraphicsPad barlowpad(gPad);
        barlowpad.Frame("barlowframe_%d", "p_{t} (GeV/c)", "(default-variation)/(#sqrt{|#sigma_{var}^{2} - sigma_{default}^{2}|})", 0., ptmax, 0., 10.);
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

        output.insert({radius, defaultspec, varspec, barlow, absdiff, varratio}); 
    }

    // Create output rootfile
    std::unique_ptr<TFile> outputwriter(TFile::Open(Form("systematics_%s.root", varname.data()), "RECREATE"));
    outputwriter->cd();
    for(const auto &rbin : output){
        std::string dirname(Form("R%02d", int(rbin.fR * 10.)));
        outputwriter->mkdir(dirname.data());
        outputwriter->cd(dirname.data());
        rbin.fDefaultSpectrum->Write("DefaultSpectrum");
        rbin.fVarSpectrum->Write("VariationSpectrum");
        rbin.fRatio->makeTH1("ratioDefaultVar", "Ratio default variation")->Write("RatioSpectra");
        rbin.fBarlow->Write("barlowtest");
        rbin.fAbsDiff->Write("absdiff");
    }
}