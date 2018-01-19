#ifndef __CLING__
#include <array>
#include <map>
#include <memory>
#include <string>
#include <ROOT/TSeq.hxx>
#include <RStringView.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH2.h>
#include <TLegend.h>
#include <TPaveText.h>
#endif

void makeComparisonTriggersZg(std::string_view filemb, std::string_view filetg) {
    std::unique_ptr<TFile> mbreader(TFile::Open(filemb.data(), "READ")), triggerreader(TFile::Open(filetg.data()));

    const std::array<int, 3> NITER = {5, 10, 25};

    TCanvas *plot = new TCanvas("triggercomp", "Trigger comparison", 1000, 400);
    plot->Divide(3,1);

    TLegend *leg = new TLegend(0.54, 0.7, 0.93, 0.89);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextFont(42);

    TPaveText *ptlabel = new TPaveText(0.44, 0.6, 0.93, 0.65, "NDC");
    ptlabel->SetBorderSize(0);
    ptlabel->SetFillStyle(0);
    ptlabel->SetTextFont(42);
    ptlabel->AddText("60 GeV/c < p_{t,jet} < 80 (GeV/c)");
    
    for(auto ipad : ROOT::TSeqI(0, 3)) {
        plot->cd(ipad + 1);

        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.03);
        
        TH1 *axis = new TH1F(Form("axis%d", ipad), "; z_{g}; 1/N_{jet} dN_{jet}/dz_{g}", 100, 0., 0.6);
        axis->SetDirectory(nullptr);
        axis->SetStats(false);
        axis->GetYaxis()->SetRangeUser(0., 0.4);
        axis->Draw("axis");
        if(ipad == 0) {
            leg->Draw();
            ptlabel->Draw();
        }

        auto hitermb = static_cast<TH2 *>(mbreader->Get(Form("zg_unfolded_iter%d.root", NITER[ipad])));
        int binpt = hitermb->GetYaxis()->FindBin(70);
        auto hspecIterMb = hitermb->ProjectionX(Form("zg_unfolded_60_80_iter%d_mb", NITER[ipad]), binpt, binpt);
        hspecIterMb->Scale(1./hspecIterMb->Integral());
        hspecIterMb->SetDirectory(nullptr);
        hspecIterMb->SetMarkerColor(kRed);
        hspecIterMb->SetLineColor(kRed);
        hspecIterMb->SetMarkerStyle(24);
        hspecIterMb->Draw("epsame");
        if(ipad == 0) leg->AddEntry(hspecIterMb, "INT7", "lep");

        auto hitertrg = static_cast<TH2 *>(triggerreader->Get(Form("zg_unfolded_iter%d.root", NITER[ipad])));
        auto hspecIterTrg = hitertrg->ProjectionX(Form("zg_unfolded_60_80_iter%d_EJ2", NITER[ipad]), binpt, binpt);
        hspecIterTrg->Scale(1./hspecIterTrg->Integral());
        hspecIterTrg->SetDirectory(nullptr);
        hspecIterTrg->SetMarkerColor(kBlue);
        hspecIterTrg->SetLineColor(kBlue);
        hspecIterTrg->SetMarkerStyle(25);
        hspecIterTrg->Draw("epsame");
        if(ipad == 0) leg->AddEntry(hspecIterTrg, "EJ2", "lep");

        auto iterlabel = new TPaveText(0.15, 0.15, 0.45, 0.22, "NDC");
        iterlabel->SetBorderSize(0);
        iterlabel->SetFillStyle(0);
        iterlabel->SetTextFont(42);
        iterlabel->AddText(Form("N_{iter} = %d", NITER[ipad]));
        iterlabel->Draw();

        gPad->Update();
    }
    plot->cd();
    plot->Update();
}