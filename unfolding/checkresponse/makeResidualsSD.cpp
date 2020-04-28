#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

void makeResidualsSD(const char *filename = "AnalysisResults.root", const char *observable = "Zg"){
    double kVerySmall = 1e-5;
    std::vector<double> partptbinning = {0, 20, 30, 40, 50, 60, 80, 100, 120, 140, 160, 180, 200, 240, 500};
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));

    std::map<std::string, std::string> parttitles = {{"Zg", "z_{g,part}"}, {"Rg", "R_{g,part}"}, {"Thetag", "#Theta_{g,part}"}},
                                       dettitles = {{"Zg", "z_{g,det}"}, {"Rg", "R_{g,det}"}, {"Thetag", "#Theta_{g,det}"}};

    std::vector<Color_t> colors = {kRed, kBlue, kGreen, kOrange, kCyan, kMagenta, kGray, kTeal, kViolet, kAzure};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28, 29, 30};

    auto style = [](Color_t col, Style_t marker) {
        return [col, marker] (auto obj) {
            obj->SetMarkerColor(col);
            obj->SetMarkerStyle(marker);
            obj->SetLineColor(col);
        };
    };

    auto plot = new ROOT6tools::TSavableCanvas(Form("%sResiduals", observable), Form("%s residuals", observable), 1200, 800);
    plot->Divide(3,2);

    TLegend *leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.89, 0.89);

    int ipad = 1;
    for(auto R : ROOT::TSeqI(2, 7)){
        std::string rstring(Form("R%02d", R)),
                    rtitle(Form("R = %.1f", double(R)/10.));
        plot->cd(ipad);
        auto parttitle = parttitles[observable],
             dettitle = dettitles[observable];
        std::string resname = Form("(%s - %s) / %s", dettitle.data(), parttitle.data(), parttitle.data());
        (new ROOT6tools::TAxisFrame(Form("ResidualsFrame%s", rstring.data()), resname.data(), "Prob. density", -1., 1., 0., 0.15))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.5, 0.89, rtitle.data()))->Draw();

        reader->cd(Form("SoftDropResponse_FullJets_%s_INT7", rstring.data()));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto reshist2d = static_cast<TH2 *>(histlist->FindObject(Form("h%sResidualsNormalized", observable)));

        int icol = 0, imrk = 0;
        for(auto ipt = 0; ipt < partptbinning.size() - 1; ipt++) {
            int binmin = reshist2d->GetXaxis()->FindBin(partptbinning[ipt] + kVerySmall),
                binmax = reshist2d->GetXaxis()->FindBin(partptbinning[ipt+1] - kVerySmall);
            auto slice = reshist2d->ProjectionY(Form("residuals%s_%s_%d_%d", observable, rstring.data(), int(partptbinning[ipt]), int(partptbinning[ipt+1])), binmin, binmax);
            slice->SetDirectory(nullptr);
            slice->Scale(1./slice->Integral());
            style(colors[icol], markers[imrk])(slice);
            slice->Draw("epsame");
            if(R==2) leg->AddEntry(slice, Form("%.1f GeV/c < p_{t,part} < %1.f GeV/c", partptbinning[ipt], partptbinning[ipt+1]), "lep");
            icol++;
            imrk++;
            if(icol == colors.size()) icol = 0;
            if(imrk == markers.size()) imrk = 0;
        }
        ipad++;
    }

    plot->cd(ipad);
    leg->Draw();

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}