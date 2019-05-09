
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
    TH1*                fAbsDiff;
    Ratio*              fRatio;

    bool operator<(const outputdata &other) const { return fR < other.fR; }
    bool operator==(const outputdata &other) const { return TMath::Abs(fR - other.fR) < 1e-6; }
};

std::map<int, TH1 *> getCorrected(const std::string_view filename, const std::string_view varname, int regularization  = 4) {
    std::vector<std::string> spectra = {Form("normalized_reg%d", regularization)};
    std::map<int, TH1 *> result;
    JetSpectrumReader reader(filename, spectra);
    for(auto r : reader.GetJetSpectra().GetJetRadii()) {
        std::cout << "Read " << r << std::endl;
        auto spec = reader.GetJetSpectrum(r, spectra[0]);  
        spec->SetNameTitle(Form("Spec%s_R%02d", varname.data(), int(r*10.)), varname.data());
        spec->SetDirectory(nullptr);
        result[int(r*10.)] = spec;
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

void testVariation1DCorrelated(const std::string_view varname, 
                               const std::string_view vartitle, 
                               const std::string_view filesuperset, 
                               const std::string_view filesubset, 
                               bool defaultIsSubset, 
                               int regularization = 4){
    std::string varnamesubset = (defaultIsSubset ? "default" : varname.data()),
                varnamesuperset = (defaultIsSubset ? varname.data() : "default");
    auto specSuperset = getCorrected(filesuperset, varnamesuperset , regularization), specSubset = getCorrected(filesubset, varnamesubset, regularization);
    Style defaultstyle{kRed, 24}, varstyle{kBlue, 25};

    std::set<outputdata> output;
    double ptmax = 350.;
    for(double r = 0.2; r <= 0.6; r+= 0.1){
        printf("Doing r=%.1f\n", r);
        std::string rstring(Form("R%02d", int(r*10.)));
        auto compplot = new ROOT6tools::TSavableCanvas(Form("comp_%s_%s", varname.data(), rstring.data()), Form("Comparison %s, R = %.1f", varname.data(), r), 1200, 1000);
        compplot->Divide(3,1);

        compplot->cd(1);
        GraphicsPad specpad(gPad);
        specpad.Logy();
        specpad.Frame(Form("compframe_%s", rstring.data()), "p_{t} (GeV/c)", "dsigma/(dp_{t} d#eta) (mb/(GeV/c))", 0., ptmax, 1e-10, 1e-1);
        specpad.Label(0.15, 0.15, 0.45, 0.21, Form("jets, R=%.1f", r));
        specpad.Legend(0.35, 0.7, 0.89, 0.89);

        auto foundSuper = specSuperset.find(int(r*10.)),
             foundSub = specSubset.find(int(r*10.)); 
        if(foundSuper == specSuperset.end()) std::cout << "super " << r << " not found" << std::endl;
        if(foundSub == specSubset.end()) std::cout << "sub " << r << " not found" << std::endl;
        auto superspec = foundSuper->second,
             subspec = foundSub->second,
             defaultspec = defaultIsSubset ? subspec : superspec,
             varspec = defaultIsSubset ? superspec : subspec;
        specpad.Draw<TH1>(defaultspec, defaultstyle, "Default");
        specpad.Draw<TH1>(varspec, varstyle, vartitle);

        compplot->cd(2);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("ratioframe_%s", rstring.data()), "p_{t} (GeV/c)", "variation / default", 0., ptmax, 0.5, 1.5);
        Ratio *varratio = new Ratio(superspec, subspec, "b");
        ratiopad.Draw<Ratio>(varratio, defaultstyle);

        compplot->cd(3);
        GraphicsPad barlowpad(gPad);
        barlowpad.Frame("barlowframe_%d", "p_{t} (GeV/c)", "(default-variation)/(#sqrt{|#sigma_{var}^{2} - sigma_{default}^{2}|})", 0., ptmax, 0., 10.);
        auto barlow = makeBarlow(superspec, subspec);
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

        output.insert({r, defaultspec, varspec, barlow, absdiff, varratio}); 
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