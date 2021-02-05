#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../binnings/binningPt1D.C"

std::map<int, TH1 *> readPartLevelSpectrumFromResponseMatrix(const char *file, bool doScale) {
    std::map<int, TH1 *> result;
    std::cout << "Reading " << file << std::endl;
    std::unique_ptr<TFile> reader (TFile::Open(file, "READ"));

    auto binning = getJetPtBinningNonLinTruePoor();
    for(auto R : ROOT::TSeqI(1, 7)) {
        reader->cd(Form("EnergyScaleResults_FullJet_R%02d_INT7_tc200", R));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto responsematrix = static_cast<TH2 *>(histlist->FindObject("hJetResponseFine"));
        auto ptpartTmp = std::unique_ptr<TH1>(responsematrix->ProjectionY("projectionPtPart"));
        auto ptpart = ptpartTmp->Rebin(binning.size()-1, Form("Ptpart_R%02d", R), binning.data());
        ptpart->SetDirectory(nullptr);
        if(doScale) {
            auto histXec = static_cast<TH1 *>(histlist->FindObject("fHistXsection")),
                 histNtrials = static_cast<TH1 *>(histlist->FindObject("fHistTrials"));
            auto weight = histXec->GetBinContent(1)/histNtrials->GetBinContent(1);
            ptpart->Scale(weight);
        }
        ptpart->Scale(1., "width");
        result[R] = ptpart;
    }

    return result;
}

void comparePartLevelMinBiasPtHard(const char *filemb, const char *filepthard) {
    auto spectraMB = readPartLevelSpectrumFromResponseMatrix(filemb, true),
         spectraPtHard = readPartLevelSpectrumFromResponseMatrix(filepthard, false);
    auto style = [](Color_t col, Style_t mrk) {
        return [col, mrk](auto obj) {
            obj->SetMarkerColor(col);
            obj->SetMarkerStyle(24);
            obj->SetLineColor(col);
        };
    };
    auto plot = new ROOT6tools::TSavableCanvas("compPtPartMinBiasPtHard", "Comparison min. bias / pt-hard", 1200, 600);
    plot->Divide(6,2);
    int icol = 0;
    for(auto R : ROOT::TSeqI(1, 7)) {
        plot->cd(icol+1);
        gPad->SetLogy();
        (new ROOT6tools::TAxisFrame(Form("specframeR%02d", R), "p_{t, part} (GeV)", "d#sigma/dp_{t,part} (mb/(GeV/c))", 0., 50., 1e-5, 100.))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.35, 0.22, Form("R=%.1f", double(R)/10.)))->Draw();
        TLegend *leg(nullptr);
        if(icol == 0){
            leg = new ROOT6tools::TDefaultLegend(0.35, 0.7, 0.89, 0.89);
            leg->Draw();
        }
        auto hMB = spectraMB[R],
             hPtHard = spectraPtHard[R];
        style(kRed, 24)(hMB);
        style(kBlue, 25)(hPtHard);
        hMB->Draw("epsame");
        hPtHard->Draw("epsame");
        if(leg) {
            leg->AddEntry(hMB, "PYTHIA8 min. bias", "lep");
            leg->AddEntry(hPtHard, "PYTHIA8 jet-jet", "lep");
        }
        plot->cd(icol+1+6);
        (new ROOT6tools::TAxisFrame(Form("specframeR%02d", R), "p_{t, part} (GeV)", "JJ/MB", 0., 50., 0.5, 1.2))->Draw("axis");
        auto ratio = static_cast<TH1 *>(hPtHard->Clone(Form("JJOverMB_R%02d", R)));
        ratio->SetDirectory(nullptr);
        ratio->Divide(hMB);
        style(kBlack, 20)(ratio);
        ratio->Draw("epsame");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}