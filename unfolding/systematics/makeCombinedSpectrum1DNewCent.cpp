#include "../../struct/Restrictor.cxx"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../meta/stl.C"

TGraphAsymmErrors *makeErrors(TH1 *spec, TH1 *sys) {
    TGraphAsymmErrors *result = new TGraphAsymmErrors;
    for(auto b : ROOT::TSeqI(0, spec->GetXaxis()->GetNbins())){
        double val = spec->GetBinContent(b+1), 
               ex = spec->GetXaxis()->GetBinWidth(b+1)/2., 
               ey = val * sys->GetBinContent(b+1);
        result->SetPoint(b, spec->GetXaxis()->GetBinCenter(b+1), val);
        result->SetPointError(b, ex, ex, ey, ey);
    }
    return result;
}

std::map<int, TH1 *> readSpectraCent(const char *filename, const char *tag, int iteration){
    std::map<int, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    for(auto R : ROOT::TSeqI(2, 7)) {
        reader->cd(Form("R%02d/reg%d", R, iteration));
        auto spectrum = static_cast<TH1 *>(gDirectory->Get(Form("normalized_reg%d", iteration)));
        spectrum->SetDirectory(nullptr);
        spectrum->SetName(Form("fullycorrected_R%02d_%s_iter%d", R, tag, iteration));
        result[R] = spectrum;
    }
    return result;
}


void makeCombinedSpectrum1DNewCent(const char *cent, const char *correlatedSys, const char *shapeSys) {
    std::unique_ptr<TFile> correader(TFile::Open(correlatedSys, "READ")),
                           shapereader(TFile::Open(shapeSys, "READ"));
    std::unique_ptr<TFile> writer(TFile::Open("jetspectrum.root", "RECREATE"));
    auto centspectra = readSpectraCent(cent, "cent", 6);
    Restrictor reported(15., 320.);
    for(auto r : ROOT::TSeqI(2, 7)){
        std::string rstring(Form("R%02d", r));
        auto spechist = reported(centspectra[r]);
        correader->cd(rstring.data());
        std::unique_ptr<TH1> corrUncertainty(reported(static_cast<TH1 *>(gDirectory->Get("combinedUncertainty"))));
        spechist->SetDirectory(nullptr);
        corrUncertainty->SetDirectory(nullptr);
        shapereader->cd(rstring.data());
        std::unique_ptr<TH1> shapeUncertainty(reported(static_cast<TH1 *>(gDirectory->Get("combinedUncertainty"))));
        shapeUncertainty->SetDirectory(nullptr);
        std::cout << spechist->GetName() << std::endl;
        std::cout << corrUncertainty->GetName() << std::endl;
        std::cout << shapeUncertainty->GetName() << std::endl;
        auto outcorr = makeErrors(spechist, corrUncertainty.get()),
             outshape = makeErrors(spechist, shapeUncertainty.get());
        spechist->SetNameTitle(Form("stat_%s", rstring.data()), Form("Spectrum with stat. uncertainty %s", rstring.data()));
        outcorr->SetNameTitle(Form("correlatedUncertainty_%s", rstring.data()), Form("Correlated uncertainty %s", rstring.data()));
        outshape->SetNameTitle(Form("shapeUncertainty_%s", rstring.data()), Form("Shape uncertainty %s", rstring.data()));

        writer->mkdir(rstring.data());
        writer->cd(rstring.data());
        spechist->Write();
        outcorr->Write();
        outshape->Write();
    }
}