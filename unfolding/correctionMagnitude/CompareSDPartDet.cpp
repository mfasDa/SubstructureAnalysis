#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/unfolding.C"

void CompareSDPartDet(const char *inputfile = "AnalysisResults.root", const char *observable = "Zg") {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ"));    
    std::vector<std::string> triggers = {"EJ1", "EJ2", "INT7"};
    std::vector<std::string> filetypes = {"pdf", "png", "eps"};
    std::map<std::string, std::string> vartitles = {{"Zg", "z_{g}"}, {"Rg", "r_{g}"}, {"Nsd", "n_{SD}"}, {"Thetag", "#Theta_{g}"}};
    std::map<std::string, double> xrange = {{"Zg", 0.55}, {"Rg", 0.65}, {"Nsd", 20}, {"Thetag", 1.}};
    auto obstitle = vartitles[observable].data();
    auto varxrange = xrange[observable];
    
    for(auto R : ROOT::TSeqI(2,7)){
        for(auto trg : triggers){
            reader->cd(Form("SoftDropResponse_FullJets_R%02d_%s", R, trg.data()));
            auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
            auto responsematrix = static_cast<RooUnfoldResponse *>(histlist->FindObject(Form("h%sResponse", observable)));
            auto htrue = responsematrix->Htruth();
            int iplot = 0, ncol = 0;
            auto plot = new TCanvas(Form("CorrectionMagnitude%s_R%02d_%s_%d", observable, R, trg.data(), iplot), Form("Correction Magnitude %s for R=%.1f for trigger %s (%d)", obstitle, double(R)/10., trg.data(), iplot), 1200, 800);
            plot->Divide(4,2);
            for(auto ib = 0; ib < htrue->GetYaxis()->GetNbins(); ib++) {
                auto ptmin = htrue->GetYaxis()->GetBinLowEdge(ib+1),
                     ptmax = htrue->GetYaxis()->GetBinUpEdge(ib+1);
                auto hslice = sliceResponseObservableBase(responsematrix->Hresponse(), (TH2 *)htrue, (TH2 *)responsematrix->Hmeasured(), "Zg", ib);
                hslice->SetDirectory(nullptr);
                auto hzgmeasured = hslice->ProjectionX(),
                     hzgtrue = hslice->ProjectionY();
                hzgmeasured->SetDirectory(nullptr);
                hzgtrue->SetDirectory(nullptr);
                hzgmeasured->Scale(1./hslice->Integral());
                hzgtrue->Scale(1./hslice->Integral());
                auto hratio = static_cast<TH1 *>(hzgmeasured->Clone(Form("%smeasured_true_R%02d_ptm%d_%s", observable, R, int(ptmin), trg.data())));
                hratio->SetDirectory(nullptr);
                hratio->Divide(hzgtrue);
                hzgmeasured->SetMarkerColor(kRed);
                hzgmeasured->SetLineColor(kRed);
                hzgmeasured->SetMarkerStyle(24);
                hzgtrue->SetMarkerColor(kBlue);
                hzgtrue->SetLineColor(kBlue);
                hzgtrue->SetMarkerStyle(25);
                hratio->SetMarkerColor(kBlack);
                hratio->SetLineColor(kBlack);
                hratio->SetMarkerStyle(20);

                if(ncol == 4) {
                   for(auto ft : filetypes) plot->SaveAs(Form("%s.%s", plot->GetName(), ft.data())); 
                   iplot++;
                   ncol=0;
                   plot = new TCanvas(Form("CorrectionMagnitude%s_R%02d_%s_%d", observable, R, trg.data(), iplot), Form("Correction Magnitude (%s) for R=%.1f for trigger %s (%d)", obstitle, double(R)/10., trg.data(), iplot), 1200, 800);
                   plot->Divide(4,2);
                } 
                plot->cd(ncol+1);
                auto frame = new TH1F(Form("frame_R%02d_ptm%d_%s", R, int(ptmin), trg.data()), Form("; %s; Prob. density", obstitle), 100, 0., varxrange);
                frame->SetDirectory(nullptr);
                frame->SetStats(false);
                frame->GetYaxis()->SetRangeUser(0., 0.4);
                frame->Draw("axis");

                hzgtrue->Draw("epsame");
                hzgmeasured->Draw("epsame");

                auto label = new TPaveText(0.15, 0.8, 0.89, 0.89, "NDC");
                label->SetBorderSize(0);
                label->SetFillStyle(0);
                label->SetTextFont(42);
                label->AddText(Form("%.1f GeV/c < p_{t,part} < %.1f GeV/c", ptmin, ptmax));
                label->Draw();
                if(ib == 0) {
                    auto trglabel = new TPaveText(0.4, 0.7, 0.89, 0.89, "NDC");
                    trglabel->SetBorderSize(0);
                    trglabel->SetFillStyle(0);
                    trglabel->SetTextFont(42);
                    trglabel->Draw();

                    auto leg = new TLegend(0.55, 0.4, 0.89, 0.7);
                    leg->SetBorderSize(0);
                    leg->SetFillStyle(0);
                    leg->SetTextFont(42);
                    leg->AddEntry(hzgtrue, "part. level", "lep");
                    leg->AddEntry(hzgmeasured, "det. level", "lep");
                    leg->Draw();
                }

                plot->cd(ncol+1+4);
                auto ratioframe = new TH1F(Form("ratioframe_R%02d_ptm%d_%s", R, int(ptmin), trg.data()), Form("; %s; det./part. level", obstitle), 100, 0., varxrange);
                ratioframe->SetDirectory(nullptr);
                ratioframe->SetStats(false);
                ratioframe->GetYaxis()->SetRangeUser(0., 2.);
                ratioframe->Draw("axis");
                hratio->Draw("epsame");
                ncol++;
            }
            for(auto ft : filetypes) plot->SaveAs(Form("%s.%s", plot->GetName(), ft.data()));  
        }
    }
}