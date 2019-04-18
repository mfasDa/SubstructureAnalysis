#include "../helpers/graphics.C"
#include "../unfolding/binnings/binningPt1D.C"

std::map<double, TH1 *> readRaw(const std::string_view filename, const std::string_view trigger){
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto binning = getJetPtBinningNonLinSmearUltra();
    for(auto r = 0.2; r <= 0.6; r += 0.1) {
        std::string rstring = Form("R%02d", int(r*10.));
        reader->cd(Form("JetSpectrum_FullJets_%s_%s", rstring.data(), trigger.data()));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto hsparse = static_cast<THnSparse *>(histlist->FindObject("hJetTHnSparse"));
        int clusterbin = 1;     // No clusters in MC
        hsparse->GetAxis(5)->SetRange(clusterbin, clusterbin);
        auto rawhisttmp = hsparse->Projection(1);
        auto rawhist = rawhisttmp->Rebin(binning.size()-1, Form("RawJetSpectrum_FullJets_R%02d_%s", int(r*10.), trigger.data()), binning.data());
        rawhist->SetDirectory(nullptr);
        rawhist->Scale(1., "width");
        result[r] = rawhist;
    }
    return result;
} 

void comparisonRawSpectraMC(const std::string_view trigger = "INT7"){
    auto datawith = readRaw("withTRD/mc/AnalysisResults.root", trigger), 
         datawo = readRaw("withoutTRD/mc/AnalysisResults.root", trigger);
    auto plot = new ROOT6tools::TSavableCanvas(Form("compTRDrawMC%s", trigger.data()), Form("comparison TRD raw spectra (MC), %s trigger", trigger.data()), datawith.size() * 300, 700);
    plot->Divide(datawith.size(), 2);
    Style wstyle{kRed, 25}, wostyle{kBlue, 26}, ratiostyle{kBlack, 20};
    int icol = 0;
    for(auto r = 0.2; r <= 0.6; r += 0.1){
        std::string rstring = Form("R%02d", int(r*10.));
        plot->cd(icol+1);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("specframe_%s", rstring.data()), "p_{t} (GeV/c)", "d#sigma/dp_{t} (mb/(GeV/c))", 0., 300., 1e-9, 1))->Draw("axis");
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