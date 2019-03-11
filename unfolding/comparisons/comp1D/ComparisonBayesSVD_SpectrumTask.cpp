#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"

#include "../../../helpers/graphics.C"

struct spectra {
    double fR;
    std::map<int, TH1 *> fReg;

    bool operator<(const spectra &other) const { return fR < other.fR; }
    bool operator==(const spectra &other) const { return TMath::Abs(fR - other.fR) < 1e-5; }
};

std::set<spectra> readSpectraIterations(std::string_view inputfile) {
    std::set<spectra> result;
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    for(auto rad : *reader->GetListOfKeys()){
        double rval = double(std::stoi(std::string(rad->GetName()).substr(1,2)))/10.;
        spectra rspec;
        rspec.fR = rval;

        reader->cd(rad->GetName());
        auto basedir = gDirectory;
        for(auto reg : *gDirectory->GetListOfKeys()){
            if(std::string_view(reg->GetName()).find("reg") == std::string::npos) continue;
            int regval = std::stoi(std::string(reg->GetName()).substr(3));
            basedir->cd(reg->GetName());
            auto hist = static_cast<TH1 *>(gDirectory->Get(Form("normalized_reg%d", regval)));
            hist->SetDirectory(nullptr);
            rspec.fReg[regval] = hist;
        }

        result.insert(rspec);
    }
    return result;
}
 
void ComparisonBayesSVD_SpectrumTask(const std::string_view svdfile, const std::string_view bayesfile){
    auto svddata = readSpectraIterations(svdfile),
         bayesdata = readSpectraIterations(bayesfile);
    int nrad = svddata.size();

    const int regSVD = 4, regBayes = 4;

    auto plot = new ROOT6tools::TSavableCanvas("comparisonBayesSvd", "Comparison Bayes - SVD", 300 * nrad, 700);
    plot->Divide(nrad, 2);
    Style svdstyle{kRed, 24}, bayesstyle{kBlue, 25};

    int currentcol = 0;
    for(auto svdit = svddata.begin(), bayesit = bayesdata.begin(); svdit != svddata.end(); ++svdit, ++bayesit) {
        plot->cd(1+currentcol);
        gPad->SetLogy();
        std::string rstring(Form("R%02d", int(svdit->fR * 10.)));
        (new ROOT6tools::TAxisFrame(Form("specframe%s", rstring.data()), "p_{t} (GeV/c)", "d#sigma/(dp_{t}dy) (mb/(GeV/c))", 0., 200, 1e-9, 100))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("R = %.1f", svdit->fR)))->Draw();
        TLegend *leg(nullptr);
        if(!currentcol) {
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
            leg->Draw();
        }

        auto specSVD = svdit->fReg.find(regSVD)->second,
            specBayes = bayesit->fReg.find(regBayes)->second;
        svdstyle.SetStyle<TH1>(*specSVD);
        bayesstyle.SetStyle<TH1>(*specBayes);
        specSVD->Draw("epsame");
        specBayes->Draw("epsame");
        if(leg) {
            leg->AddEntry(specSVD, Form("SVD, reg=%d", regSVD), "lep");
            leg->AddEntry(specBayes, Form("Bayes, reg=%d", regBayes), "lep");
        }

        plot->cd(1+currentcol+nrad);
        (new ROOT6tools::TAxisFrame(Form("ratframe%s", rstring.data()), "p_{t} (GeV/c)", "Bayes / SVD", 0., 200, 0.5, 1.5))->Draw("axis");
        auto ratioBayesSVD = static_cast<TH1*>(specBayes->Clone(Form("RatioBayesSVD_%s", rstring.data())));
        ratioBayesSVD->SetDirectory(nullptr);
        ratioBayesSVD->Divide(specSVD);
        Style{kBlack, 20}.SetStyle<TH1>(*ratioBayesSVD);
        ratioBayesSVD->Draw("epsame");

        currentcol++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}