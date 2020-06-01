#include "../../meta/stl.C"
#include "../../meta/root.C"

void makeKinematicEfficiencyUnfoldedFile(const char *observable, const char *inputfile) {
	std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ"));

    auto style = [](Color_t col, Style_t mrk){
        return [col, mrk] (auto obj) {
            obj->SetMarkerColor(col);
            obj->SetLineColor(col);
            obj->SetMarkerStyle(mrk);
        };
    };

    std::vector<Color_t> colors = {kRed, kBlue, kGreen, kBlack, kMagenta, kCyan, kViolet, kOrange, kTeal, kGray};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28, 29, 30};

    auto plot = new TCanvas(Form("EffKine%s", observable), Form("Kinematic efficiency for %s", observable), 1200, 800);
    plot->Divide(3,2);

    reader->cd(observable);
    auto basedir = gDirectory;

    int ipad = 1;
	for(auto R : ROOT::TSeqI(2, 7)){
		basedir->cd(Form("R%02d", R));
        gDirectory->cd("response");
        plot->cd(ipad);
        auto heff = static_cast<TH2 *>(gDirectory->Get(Form("EffKine%s_R%02d", observable, R)));

        TH1 *frame = nullptr;
        TLegend *leg(nullptr);

        int currentmarker = 0, currentcolor = 0;
        
        for(auto ipt : ROOT::TSeqI(0, heff->GetYaxis()->GetNbins())) {
            double ptmin = heff->GetYaxis()->GetBinLowEdge(ipt+1),
                   ptmax = heff->GetYaxis()->GetBinUpEdge(ipt+1);
            auto effslice = heff->ProjectionX(Form("SliceEff%s_%d_%d", observable, int(ptmin), int(ptmax)), ipt+1, ipt+1);
            effslice->SetDirectory(nullptr);
            
            if(!frame) {
                frame = new TH1F(Form("FrameR%02d", R), Form("; %s; #epsilon_{kine}", observable), 100, effslice->GetXaxis()->GetBinLowEdge(1), effslice->GetXaxis()->GetBinUpEdge(effslice->GetXaxis()->GetNbins()));
                frame->SetDirectory(nullptr);
                frame->SetStats(false);
                frame->GetYaxis()->SetRangeUser(0., 2.);
                frame->Draw("axis");

                TPaveText *rlabel = new TPaveText(0.15, 0.9, 0.45, 0.95, "NDC");
                rlabel->SetBorderSize(0);
                rlabel->SetFillStyle(0);
                rlabel->SetTextFont(42);
                rlabel->AddText(Form("R=%.1f", double(R)/10.));
                rlabel->Draw();
                
                if(ipad == 1) {
                    leg = new TLegend(0.15, 0.55, 0.89, 0.89);
                    leg->SetBorderSize(0);
                    leg->SetFillStyle(0);
                    leg->SetTextFont(42);
                    leg->SetNColumns(2);
                    leg->Draw();
                }
            }

            style(colors[currentcolor], markers[currentmarker])(effslice);
            effslice->Draw("epsame");
            if(leg) leg->AddEntry(effslice, Form("%.1f GeV/c < p_{t,j} < %.1f", ptmin, ptmax), "lep");
            currentcolor++;
            currentmarker++;
            if(currentcolor == colors.size()) currentcolor = 0;
            if(currentmarker == markers.size()) currentmarker = 0;
        }
        ipad++;
	}

    plot->cd();
    plot->Update();
    std::vector<std::string> filetypes = {"eps", "png", "pdf"};
    for(auto ft : filetypes) plot->SaveAs(Form("%s.%s", plot->GetName(), ft.data()));
}