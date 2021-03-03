#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"

struct mystyle {
    Color_t col;
    Style_t mrk;
};

struct spectrum_ktmin {
    int ktmin;
    TH1 *spectrum;

    bool operator<(const spectrum_ktmin &other) const { return ktmin < other.ktmin; }
    bool operator==(const spectrum_ktmin &other) const { return ktmin == other.ktmin; }
};

std::map<int, mystyle> makeStyles(int maxbin) {
    std::vector<Color_t> cols = {kRed, kBlue, kGreen, kOrange, kMagenta, kTeal, kViolet, kAzure, kGray};
    std::vector<Style_t> mrks = {24, 25, 26, 27, 28};
    std::map<int, mystyle> styles;
    int icol = 0, imrk = 0;
    for(auto ist : ROOT::TSeqI(1, maxbin+1)) {
        styles[ist] = {cols[icol], mrks[imrk]};
        icol++;
        imrk++;
        if(icol == cols.size()) icol = 0;
        if(imrk == mrks.size()) imrk = 0;
    }
    return styles;
}

std::map<int, TH1 *> readOutlierDists(const char *filename, int outliercut) {
    std::cout << "Reading " << filename << std::endl;
    std::map<int, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    auto neventhist = reader->Get<TH1>("hNevents"),
	 xsechist = reader->Get<TH1>("hXsection");
    auto weight = xsechist->GetBinContent(1)/neventhist->GetBinContent(1);
    reader->cd("Spectra");
    for(auto R : ROOT::TSeqI(2, 7)) {
	std::cout << "Get jet spectrum for R=" << R << std::endl;
	auto hJetSpectrum = gDirectory->Get<TH1>(Form("JetSpectrumAbsR%02d", R));
        hJetSpectrum->SetDirectory(nullptr);
        hJetSpectrum->Scale(weight);
        result[R] = hJetSpectrum;
    }
    std::cout << "Reading file done" << std::endl;
    return result;
}

std::vector<int> findKtMinDists(){
    std::vector<int> result;
    std::unique_ptr<TObjArray> dircont(gSystem->GetFromPipe("ls -1").Tokenize("\n"));
    for(auto cont : TRangeDynCast<TObjString>(dircont.get())) {
        auto &cstr = cont->String();
        if(cstr.IsDigit()) result.push_back(cstr.Atoi());
    }
    std::sort(result.begin(), result.end(), std::less<int>());
    return result;
}

void compareJetSpectrumKtMin_standalone(const char *filename = "AnalysisResults.root") {
    auto plot = new ROOT6tools::TSavableCanvas("ComparisonJetskTMin", "Comparison jet kt-min", 1200, 800);
    plot->Divide(3,2);
    int ipad = 1;
    for(auto R : ROOT::TSeqI(2, 7)) {
        plot->cd(ipad);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("outlierJetR%02d", R), "p_{t,j}", "d#sigma/dp_{t,j} (mb/(GeV/c))", 0., 500, 1e-10, 1e1))->Draw();
        (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.5, 0.89, Form("R = %.1f", double(R)/10.)))->Draw();
        ipad++;
    }

    auto style = [](Color_t col, Style_t mrk){
        return [col, mrk] (auto obj) {
            obj->SetMarkerColor(col);
            obj->SetLineColor(col);
            obj->SetMarkerStyle(mrk);
        };
    };


    TLegend *leg = new ROOT6tools::TDefaultLegend(0.45, 0.45, 0.89, 0.89);

    std::map<int, mystyle> allstyles = makeStyles(20);
    std::map<int, mystyle> ktminstyles;
    int styleID = 1;
    std::map<int, std::vector<spectrum_ktmin>> ktminspectra;
    for(auto R : ROOT::TSeqI(2, 7)) ktminspectra[R] = std::vector<spectrum_ktmin>();
    for(auto pt : findKtMinDists()) {
        auto hists = readOutlierDists(Form("%02d/%s", pt, filename), -1);
        auto ptstyle = allstyles[styleID];
        ktminstyles[pt] = ptstyle;
        styleID++;
        ipad = 1;
        for(auto R : ROOT::TSeqI(2, 7)) {
            plot->cd(ipad);
            gPad->SetLeftMargin(0.15);
            gPad->SetRightMargin(0.05);
            auto hist = hists[R];
            style(ptstyle.col, ptstyle.mrk)(hist);
            hist->Draw("epsame");
            if(R == 2) leg->AddEntry(hist, Form("k_{t,min} = %d GeV/c", pt), "lep");
            ipad++;
            ktminspectra[R].push_back({pt, hist});
        }
    }

    std::string mbfile = Form("mb/%s", filename);
    if(!gSystem->AccessPathName(mbfile.data())){
        auto mbhists =  readOutlierDists(mbfile.data(), -1);
        ipad = 1;
        ipad = 1;
        for(auto R : ROOT::TSeqI(1,7)) {
            plot->cd(ipad);
            gPad->SetLeftMargin(0.15);
            gPad->SetRightMargin(0.05);
            auto hist = mbhists[R];
            style(kBlack, 20)(hist);
            hist->Draw("epsame");
            if(R == 2) leg->AddEntry(hist, "min. bias", "lep");
            ipad++;
        }
    }

    // Add legend at the end
    plot->cd(6);
    leg->Draw();
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());

    // make binary ratios
    auto ratioplot = new ROOT6tools::TSavableCanvas("RatiosJetskTMin", "Comparison jet kt-min", 1200, 800);
    ratioplot->Divide(3,2);
    ipad = 1;
    auto ratiolegend = new ROOT6tools::TDefaultLegend(0.45, 0.45, 0.89, 0.89);
    for(auto R : ROOT::TSeqI(2, 7)){
        ratioplot->cd(ipad); 
        (new ROOT6tools::TAxisFrame(Form("ratioframeR%02d", R), "p_{t} (GeV/c)", "Ratio k_{t,min}", 0., 500., 0., 1.2))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.45, 0.89, Form("R = %1.f", double(R)/10.)))->Draw();
        auto spectra = ktminspectra[R];
        std::sort(spectra.begin(), spectra.end(), std::less<spectrum_ktmin>());
        for(auto index = 0; index < spectra.size() - 1; index++) {
            auto specdenom = spectra[index],
                 specnum = spectra[index+1];
            auto ratio = static_cast<TH1 *>(specnum.spectrum->Clone(Form("ratio_%d_%d_R%02d", specnum.ktmin, specdenom.ktmin, R)));
            ratio->SetDirectory(nullptr);
            ratio->Divide(specnum.spectrum, specdenom.spectrum, 1., 1., "b");
            auto ratiostyle = ktminstyles[specnum.ktmin];
            style(ratiostyle.col, ratiostyle.mrk)(ratio);
            ratio->Draw("epsame");
            if(R == 2) ratiolegend->AddEntry(ratio, Form("%d GeV/c / %d GeV/c", specnum.ktmin, specdenom.ktmin), "lep");
        }
        ipad++;
    }
    // Add legend at the end
    ratioplot->cd(6);
    ratiolegend->Draw();
    ratioplot->cd();
    ratioplot->Update();
    ratioplot->SaveCanvas(ratioplot->GetName());
}

