#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../helpers/pthard.C"
#include "../helpers/substructuretree.C"
#include "../helpers/root.C"

std::pair<TH2 *, TH2 *> getZLeadingDist(double R, bool data) {
    std::stringstream filename;
    filename <<  (data ? "data/merged_17" : "mc/merged_calo") << "/JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(R*10.) << "_" << (data ? "EJ1" : "INT7_merged") << ".root";
    ROOT::RDataFrame specframe(GetNameJetSubstructureTree(filename.str()), filename.str());
    std::vector<ROOT::RDF::RResultPtr<TH2D>> dists;
    if(!data) {
        const std::string weightbranch = "PythiaWeight";
        dists.push_back(specframe.Filter([](double ptsim, int pthardbin) { return !IsOutlierFast(ptsim, pthardbin); }, {"PtJetSim", "PtHardBin"}).Histo2D({Form("ZLeadingChargedR%02d", int(R*10.)), "z_{l,ch} vs. p_{t}", 5, 100, 200, 100, 0., 1., }, "PtJetRec", "ZLeadingChargedRec", weightbranch.data()));
        dists.push_back(specframe.Filter([](double ptsim, int pthardbin) { return !IsOutlierFast(ptsim, pthardbin); }, {"PtJetSim", "PtHardBin"}).Histo2D({Form("ZLeadingNeutralR%02d", int(R*10.)), "z_{l,ch} vs. p_{t}", 5, 100, 200, 100, 0., 1., }, "PtJetRec", "ZLeadingNeutralRec", weightbranch.data()));
    } else {
        dists.push_back(specframe.Histo2D({Form("ZLeadingChargedR%02d", int(R*10.)), "z_{l,ch} vs. p_{t}", 5, 100, 200, 100, 0., 1., }, "PtJetRec", "ZLeadingChargedRec"));
        dists.push_back(specframe.Histo2D({Form("ZLeadingNeutralR%02d", int(R*10.)), "z_{l,ch} vs. p_{t}", 5, 100, 200, 100, 0., 1., }, "PtJetRec", "ZLeadingNeutralRec"));
    }
    return {static_cast<TH2 *>(histcopy(dists[0].GetPtr())), static_cast<TH2 *>(histcopy(dists[1].GetPtr()))};
}

void TestZLeadingCut(){
    const std::map<std::string, Color_t> colors = {{"charged", kRed}, {"neutral", kBlue}};
    const std::map<std::string, Style_t> markers = {{"data", 20}, {"mc", 24}};
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    for(auto r : jetradii) {
        auto plotR  = new ROOT6tools::TSavableCanvas(Form("ZleadingDistsDataMCR%02d", int(r*10.)), Form("z_{leading} dists for R=%.1f", r), 1200, 1000);
        plotR->Divide(3,2);

        auto histsData = getZLeadingDist(r,true),
             histsMC   = getZLeadingDist(r,false);
        int ipad(1);
        for(auto b : ROOT::TSeqI(0, histsData.first->GetXaxis()->GetNbins())) {
            TLegend *leg = nullptr;
            plotR->cd(ipad);
            gPad->SetLeftMargin(0.15);
            gPad->SetRightMargin(0.05);
            (new ROOT6tools::TAxisFrame(Form("ZframeR%02d", int(r*10.)), "z_{leading}", "1/N_{jet} N(z_{leading})", 0., 1., 0., 0.1))->Draw();
            if(ipad == 1){
                leg = new ROOT6tools::TDefaultLegend(0.55, 0.65, 0.94, 0.89);
                leg->Draw();
                (new ROOT6tools::TNDCLabel(0.75, 0.5, 0.94, 0.6, Form("R=%.1f", r)))->Draw("epsame");
            }
            double ptmin = histsData.first->GetXaxis()->GetBinLowEdge(b+1), ptmax = histsData.first->GetXaxis()->GetBinUpEdge(b+1);
            (new ROOT6tools::TNDCLabel(0.2, 0.8, 0.6, 0.89, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptmin, ptmax)))->Draw();
            auto proChargedData = histsData.first->ProjectionY(Form("sliceZLeadingChargedDataR%02d_%d_%d", int(r*10.), int(ptmin), int(ptmax)), b+1, b+1),
                 proNeutralData = histsData.second->ProjectionY(Form("sliceZLeadingNeutralDataR%02d_%d_%d", int(r*10.), int(ptmin), int(ptmax)), b+1, b+1),
                 proChargedMC = histsMC.first->ProjectionY(Form("sliceZLeadingChargedMCR%02d_%d_%d", int(r*10.), int(ptmin), int(ptmax)), b+1, b+1),
                 proNeutralMC = histsMC.second->ProjectionY(Form("sliceZLeadingNeutralMCR%02d_%d_%d", int(r*10.), int(ptmin), int(ptmax)), b+1, b+1);
            proChargedData->Scale(1./proChargedData->Integral());
            proNeutralData->Scale(1./proNeutralData->Integral());
            proChargedMC->Scale(1./proChargedMC->Integral());
            proNeutralMC->Scale(1./proNeutralMC->Integral());

            Style{colors.find("charged")->second, markers.find("data")->second}.SetStyle<TH1>(*proChargedData);
            Style{colors.find("neutral")->second, markers.find("data")->second}.SetStyle<TH1>(*proNeutralData);
            Style{colors.find("charged")->second, markers.find("mc")->second}.SetStyle<TH1>(*proChargedMC);
            Style{colors.find("neutral")->second, markers.find("mc")->second}.SetStyle<TH1>(*proNeutralMC);

            proChargedData->Draw("epsame");
            proNeutralData->Draw("epsame");
            proChargedMC->Draw("epsame");
            proNeutralMC->Draw("epsame");
            
            if(leg){
                leg->AddEntry(proChargedData, "charged, data (EJ1)", "lep");
                leg->AddEntry(proChargedMC, "charged, MC (INT7)", "lep");
                leg->AddEntry(proNeutralData, "neutral, data (EJ1)", "lep");
                leg->AddEntry(proNeutralMC, "neutral, MC (INT7)", "lep");
            }
            ipad++;
        }
        plotR->cd();
        plotR->Update();
        plotR->SaveCanvas(plotR->GetName());
    }

}