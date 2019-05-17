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

void makeCombinedRatios1D(const char *correlatedSys, const char *shapeSys) {
    std::unique_ptr<TFile> correader(TFile::Open(correlatedSys, "READ")),
                           shapereader(TFile::Open(shapeSys, "READ"));
    std::unique_ptr<TFile> writer(TFile::Open("jetspectrumratios.root", "RECREATE"));
    Restrictor reported(20., 320.);
    for(auto r : ROOT::TSeqI(3, 7)){
        std::string rstring(Form("R02R%02d", r));
        correader->cd(rstring.data());
        auto spechist = reported(static_cast<TH1 *>(gDirectory->Get("DefaultRatio")));
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