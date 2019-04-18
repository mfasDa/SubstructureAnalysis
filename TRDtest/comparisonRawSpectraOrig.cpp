#include "../helpers/graphics.C"
#include "../unfolding/binnings/binningPt1D.C"

std::map<double, TH1 *> readRaw(const std::string_view filename, const std::string_view trigger, const std::string_view sysvar){
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto binning = getJetPtBinningNonLinSmearUltra300();
    for(auto r = 0.2; r <= 0.6; r += 0.1) {
        std::string rstring = Form("R%02d", int(r*10.));
        reader->cd(Form("JetSpectrum_FullJets_%s_%s_%s", rstring.data(), trigger.data(), sysvar.data()));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        int clusterbin = 2;     // CENT cluster - require TRD
        TH1 *rawhisttmp(nullptr);
        auto hsparse = static_cast<THnSparse *>(histlist->FindObject("hJetTHnSparse"));
        if(hsparse) {
            // Method with THnSparse
            hsparse->GetAxis(5)->SetRange(clusterbin, clusterbin);
            rawhisttmp = hsparse->Projection(1);
        } else {
            auto hspec2d = static_cast<TH2 *>(histlist->FindObject("hJetSpectrum"));
            rawhisttmp = hspec2d->ProjectionY("spec2d", clusterbin, clusterbin);   
        }
        auto rawhist = rawhisttmp->Rebin(binning.size()-1, Form("RawJetSpectrum_FullJets_R%02d_%s", int(r*10.), trigger.data()), binning.data());
        rawhist->SetDirectory(nullptr);
        rawhist->Scale(1., "width");
        auto normhist = static_cast<TH1 *>(histlist->FindObject("hClusterCounterAbs"));
        rawhist->Scale(1./normhist->GetBinContent(clusterbin));
        result[r] = rawhist;
    }
    return result;
} 

void comparisonRawSpectraOrig(const std::string_view trigger = "INT7", const std::string_view sysvar = "notc"){
    auto datawith = readRaw("withTRD/data/AnalysisResults.root", trigger, sysvar), 
         datawo = readRaw("withoutTRD/data/AnalysisResults.root", trigger, sysvar);
    auto plot = new ROOT6tools::TSavableCanvas(Form("compTRDraw%s_%s", trigger.data(), sysvar.data()), Form("comparison TRD raw, %s trigger", trigger.data()), datawith.size() * 300, 700);
    plot->Divide(datawith.size(), 2);
    Style wstyle{kRed, 25}, wostyle{kBlue, 26}, ratiostyle{kBlack, 20};
    int icol = 0;
    for(auto r = 0.2; r <= 0.6; r += 0.1){
        std::string rstring = Form("R%02d", int(r*10.));
        plot->cd(icol+1);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("specframe_%s", rstring.data()), "p_{t} (GeV/c)", "dN/dp_{t} ((GeV/c)^{-1})", 0., 300., 1e-9, 1))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.4, 0.22, Form("R=%.1f", r)))->Draw();
        TLegend *leg(nullptr);
        if(!icol){
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
            leg->Draw();
            (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.45, 0.89, Form("%s trigger", trigger.data())))->Draw();
        }
        auto specwith = datawith.find(r)->second,
             specwithout = datawo.find(r)->second;
        wostyle.SetStyle<TH1>(*specwith);
        wstyle.SetStyle<TH1>(*specwithout);
        specwith->Draw("epsame");
        specwithout->Draw("epsame");
        if(leg) {
            leg->AddEntry(specwith, "with TRD", "lep");
            leg->AddEntry(specwithout, "without TRD", "lep");
        }

        plot->cd(icol+1+datawith.size());
        (new ROOT6tools::TAxisFrame(Form("ratioframe_%s", rstring.data()), "p_{t} (GeV/c)", "wo/w TRD", 0., 300., 0.5, 1.5))->Draw("axis");
        auto ratio = static_cast<TH1 *>(specwithout->Clone(Form("ratio_%s", rstring.data())));
        ratio->SetDirectory(nullptr);
        ratio->Divide(specwith);
        ratiostyle.SetStyle<TH1>(*ratio);
        ratio->Draw("epsame");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}