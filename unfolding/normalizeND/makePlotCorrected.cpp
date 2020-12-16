#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

struct ptbin {
    int ptmin;
    int ptmax;
    TH1 *hist;

    bool operator==(const ptbin &other) const { return other.ptmin == ptmin && other.ptmax == ptmax; }
    bool operator<(const ptbin &other) const { return ptmax <= other.ptmin; }
};

std::vector<int> parseName(const char * histname) {
    TString tmp(histname);
    std::unique_ptr<TObjArray> toks(tmp.Tokenize("_"));
    auto stringptmin = static_cast<TObjString *>(toks->At(2)),
         stringptmax = static_cast<TObjString *>(toks->At(3));
    return {stringptmin->String().Atoi(), stringptmax->String().Atoi()};
}

void makePlotCorrected(const char *filename = "UnfoldedSD.root", const char *probe = "Zg"){
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));

    std::vector<Color_t> colors = {kRed, kBlue, kGreen, kOrange, kMagenta, kTeal, kGray, kViolet};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28};

    auto style = [](Color_t col, Style_t mrk) {
        return [col, mrk] (TH1 *hist) {
            hist->SetMarkerColor(col);
            hist->SetMarkerStyle(mrk);
            hist->SetLineColor(col);
        };
    };

    auto plot = new ROOT6tools::TSavableCanvas(Form("ptdependence%s", probe), Form("Pt-dependence of %s", probe), 1000, 800);
    plot->Divide(3,2);

    double ptmin = 15,
           ptmax = 200;
    
    double ymax = std::string_view(probe) == "Nsd" ? 0.5 : 10.;

    int ipad = 1;
    reader->ls();
    auto basedir = gDirectory;
    auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.15, 0.89, 0.89);
    for(auto R : ROOT::TSeqI(2, 7)){
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        std::string basedirname = Form("%s/%s", probe, rstring.data());
        basedir->cd(basedirname.data());
        gDirectory->ls();
        std::set<ptbin> bins;
        TH1 *correctedhist = nullptr;
        for(auto dist : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
            auto histobj = dist->ReadObject<TH1>();
            histobj->SetDirectory(nullptr);
            if(!correctedhist) correctedhist = histobj;
            auto limits = parseName(histobj->GetName());
            bins.insert({limits[0], limits[1], histobj});
        }

        plot->cd(ipad);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        (new ROOT6tools::TAxisFrame(Form("%sframe%s", probe, rstring.data()), probe, Form("1/N_{jets} dN/d%s", probe), correctedhist->GetXaxis()->GetBinLowEdge(1), correctedhist->GetXaxis()->GetBinUpEdge(correctedhist->GetXaxis()->GetNbins()), 0., ymax))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.25, 0.78, 0.45, 0.89, rtitle.data()))->Draw();

        int icol = 0, imrk = 0;
        for(auto &[ptlow, pthigh, projected] : bins) {
            double ptcent = (ptlow + pthigh)/2.;
            if(ptcent < ptmin || ptcent > ptmax) continue;
            std::cout << "Doing bin from " << ptlow << " GeV/c to " << pthigh << " GeV/c" << std::endl;

            style(colors[icol], markers[imrk])(projected);
            projected->Draw("epsame");
            if(ipad == 1) leg->AddEntry(projected, Form("%d GeV/c < p_{t} < %d GeV/c", ptlow, pthigh), "lep");
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