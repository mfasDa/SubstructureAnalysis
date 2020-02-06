#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/roounfold.C"
#include "../../helpers/unfolding.C"

void sliceResponseMatrices(const char *filename = "responsematrix.root", const char *observable = "Zg") {
    std::map<std::string, std::string> obstitles = {{"Zg", "z_{g}"}, {"Rg", "r_{g}"}, {"Nsd", "n_{SD}"}, {"Thetag", "#Theta_{g}"}};
    std::string obstitle = obstitles[observable];

    auto style = [](Color_t color, Style_t marker) {
        return [color, marker] (auto obj) {
            obj->SetMarkerColor(color);
            obj->SetLineColor(color);
            obj->SetMarkerStyle(marker);
        };
    };

    std::map<int, Color_t> colors = {{2, kRed}, {3, kBlue}, {4, kGreen}, {5, kOrange}, {6, kMagenta}};
    std::map<int, Style_t> markers = {{2, 24}, {3, 25}, {4, 26}, {5, 27}, {6, 28}};
    std::vector<std::string> formats = {"eps", "pdf", "png"};
    
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));

    auto plotPt2D = new TCanvas("proResponsePt2D", "2D projection of the response matrix in p_{t}" , 1200, 800),
         plotSD2D = new TCanvas(Form("proResponse%s2D", observable), Form("2D projection of the response matrix in %s", observable), 1200, 800),
         plotPt1D = new TCanvas("proResponsePt1D", "1D projection of the response matrix in p_{t}" , 1000, 800);
    plotPt2D->Divide(3,2);
    plotSD2D->Divide(3,2);

    plotPt1D->Divide(2,1);
    plotPt1D->cd(1);
    gPad->SetLogy();
    auto framepart = new TH1F("Framepart", "; p_{t}^{part} (GeV/c); d#sigma/dp_{t}^{part} (mb/(GeV/c))", 100, 0., 550.);
    framepart->SetDirectory(nullptr);
    framepart->SetStats(false);
    framepart->GetYaxis()->SetRangeUser(1e-9, 100);
    framepart->Draw("axis");
    TLegend *leg1D = new TLegend(0.7, 0.4, 0.89, 0.89);
    leg1D->SetBorderSize(0);
    leg1D->SetFillStyle(0);
    leg1D->SetTextFont(42);
    leg1D->Draw();
    plotPt1D->cd(2);
    gPad->SetLogy();
    auto framedet = new TH1F("Framedet", "; p_{t}^{det} (GeV/c); d#sigma/dp_{t}^{det} (mb/(GeV/c))", 100, 0., 250.);
    framedet->SetDirectory(nullptr);
    framedet->SetStats(false);
    framedet->GetYaxis()->SetRangeUser(1e-9, 100);
    framedet->Draw("axis");

    int ipadLarge = 1;
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", R)),
                    rtitle(Form("R = %.1f", double(R)/10.));
        reader->cd(Form("Response_%s", rstring.data()));
        auto responsematrix = static_cast<RooUnfoldResponse *>(gDirectory->Get(Form("Responsematrix%s_%s", observable, rstring.data())));
        auto slicedPt = sliceRepsonsePt(*responsematrix, observable);
        slicedPt->SetDirectory(nullptr);
        slicedPt->SetStats(false);
        slicedPt->GetXaxis()->SetTitle("p_{t}^{det} (GeV/c)");
        slicedPt->GetYaxis()->SetTitle("p_{t}^{part} (GeV/c)");
        slicedPt->SetTitle(Form("Response Matrix p_{t}, %s", rtitle.data()));
        plotPt2D->cd(ipadLarge);
        slicedPt->Draw("colz");

        auto slicedSD = sliceRepsonseObservable(*responsematrix, observable);
        slicedSD->SetDirectory(nullptr);
        slicedSD->SetStats(false);
        slicedSD->GetXaxis()->SetTitle(Form("%s^{det} (GeV/c)", obstitle.data()));
        slicedSD->GetYaxis()->SetTitle(Form("%s^{part} (GeV/c)", obstitle.data()));
        slicedSD->SetTitle(Form("Response Matrix %s, %s", obstitle.data(), rtitle.data()));
        plotSD2D->cd(ipadLarge);
        slicedSD->Draw("colz");

        auto ptdet = slicedPt->ProjectionX(Form("responsePtDet_%s", rstring.data())),
             ptpart = slicedPt->ProjectionY(Form("responsePtPart_%s", rstring.data()));
        ptdet->SetDirectory(nullptr);
        ptpart->SetDirectory(nullptr);
        ptdet->Scale(1., "width");
        ptpart->Scale(1., "width");
        auto mystyle = style(colors[R], markers[R]);
        mystyle(ptdet);
        mystyle(ptpart);
        plotPt1D->cd(1);
        ptpart->Draw("epsame");
        leg1D->AddEntry(ptpart, rtitle.data(), "lep");
        plotPt1D->cd(2); 
        ptdet->Draw("epsame");

        // projection in bins of pt part
        int iplotSlice = 0, ipadSlice = 1;
        auto plotSDSlice = new TCanvas(Form("proResponse%s2D_%s_%d", observable, rstring.data(), iplotSlice), Form("Slices of the Response matrix in pt for %s (%d)", rtitle.data(), iplotSlice), 1200, 800);
        plotSDSlice->Divide(5,2);
        for(auto ipt : ROOT::TSeqI(0, ptpart->GetXaxis()->GetNbins())) {
            double ptmin = ptpart->GetXaxis()->GetBinLowEdge(ipt+1),
                   ptmax = ptpart->GetXaxis()->GetBinUpEdge(ipt+1);
            std::cout << "Doing slice " << ipt << ": " << ptmin << " - " << ptmax << std::endl;
            if(ipadSlice > 10) {
                iplotSlice++;
                ipadSlice = 1;
                plotSDSlice->cd();
                plotSDSlice->Update();
                for(auto ft : formats) plotSDSlice->SaveAs(Form("%s.%s", plotSDSlice->GetName(), ft.data()));
                plotSDSlice = new TCanvas(Form("proResponse%s2D_%s_%d", observable, rstring.data(), iplotSlice), Form("Slices of the Response matrix in pt for %s (%d)", rtitle.data(), iplotSlice), 1200, 800);
                plotSDSlice->Divide(5,2);
            }
            auto sliceSDbin = sliceRepsonseObservable(*responsematrix, observable, ipt, -1);   // response matrix not using overflow bins
            sliceSDbin->SetDirectory(nullptr);
            sliceSDbin->SetStats(false);
            sliceSDbin->SetName(Form("%sSlicePt_%d_%d_%s", observable, int(ptmin), int(ptmax), rstring.data()));
            sliceSDbin->SetTitle(Form("%s, %1.f GeV/c < p_{t}^{part} < %.1f GeV/c", rtitle.data(), ptmin, ptmax));
            sliceSDbin->GetXaxis()->SetTitle(Form("%s^{det} (GeV/c)", obstitle.data()));
            sliceSDbin->GetYaxis()->SetTitle(Form("%s^{part} (GeV/c)", obstitle.data()));
            plotSDSlice->cd(ipadSlice);
            sliceSDbin->Draw("colz");
            ipadSlice++;
        }
        plotSDSlice->cd();
        plotSDSlice->Update();
        for(auto ft : formats) plotSDSlice->SaveAs(Form("%s.%s", plotSDSlice->GetName(), ft.data()));

        ipadLarge++;
    }   

    plotPt2D->cd();
    plotPt2D->Update();
    for(auto ft : formats) plotPt2D->SaveAs(Form("%s.%s", plotPt2D->GetName(), ft.data()));
    plotSD2D->cd();
    plotSD2D->Update();
    for(auto ft : formats) plotSD2D->SaveAs(Form("%s.%s", plotSD2D->GetName(), ft.data()));
    plotPt1D->cd();
    plotPt1D->Update();
    for(auto ft : formats) plotPt1D->SaveAs(Form("%s.%s", plotPt1D->GetName(), ft.data()));
}