#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

void extractPerJetNormalizedSD_progreport(const char *filename = "UnfoldedSD.root", const char *probe = "Zg"){
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

    auto plot = new ROOT6tools::TSavableCanvas(Form("ptdependence%s_R04", probe), Form("Pt-dependence of %s", probe), 800, 600);
    plot->cd();

    int R = 4;
    std::string rstring = Form("R%02d", R),
                rtitle = Form("R = %.1f", double(R)/10.);
    (new ROOT6tools::TAxisFrame(Form("%sframe%s", probe, rstring.data()), probe, Form("1/N_{jets} dN/d%s", probe), -0.1, 1., 0., 10.))->Draw("axis");
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.25, 0.89, rtitle.data()))->Draw();
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.15, 0.89, 0.89);
    leg->Draw();

    int ipad = 1;
    const double ptmin = 15., ptmax = 200.;
    const int defaultiteration = 6;
    reader->cd(probe);
    auto basedir = gDirectory;

    basedir->cd(rstring.data());
    gDirectory->cd(Form("Iter%d", defaultiteration));
    auto correctedhist = static_cast<TH2 *>(gDirectory->Get(Form("correctedIter%d_%s_%s", defaultiteration, probe, rstring.data())));


    int icol = 0, imrk = 0;
    for(auto iptb : ROOT::TSeqI(0, correctedhist->GetYaxis()->GetNbins())) {
        auto ptlow = correctedhist->GetYaxis()->GetBinLowEdge(iptb+1),
             pthigh = correctedhist->GetYaxis()->GetBinUpEdge(iptb+1),
             ptcent = correctedhist->GetYaxis()->GetBinCenter(iptb+1);
        if(ptcent < ptmin || ptcent > ptmax) continue;
        std::cout << "Doing bin from " << ptlow << " GeV/c to " << pthigh << " GeV/c" << std::endl;
        auto projected = correctedhist->ProjectionX(Form("projection%s_%s_%d_%d", probe, rstring.data(), int(ptlow), int(pthigh)), iptb+1, iptb+1);
        std::cout << "Got projection with name " << projected->GetName() << std::endl;
        projected->SetDirectory(nullptr);
        projected->Scale(1./projected->Integral());
        projected->Scale(1., "width"); 

        style(colors[icol], markers[imrk])(projected);
        projected->Draw("epsame");
        leg->AddEntry(projected, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptlow, pthigh), "lep");
        icol++;
        imrk++;
        if(icol == colors.size()) icol = 0;
        if(imrk == markers.size()) imrk = 0;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}