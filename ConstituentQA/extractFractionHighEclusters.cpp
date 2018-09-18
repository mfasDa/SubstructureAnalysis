#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

void extractFractionHighEclusters(const std::string_view filename){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("JetConstituentQA_EJ1");    
    auto histlist = static_cast<TList *>(gDirectory->Get(Form("JetConstituentQA_fulljets_EJ1")));
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    
    std::map<double, TGraphErrors *> graphs;
    for(auto r : jetradii) {
        auto hsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hNeutralConstituentsfulljets_R%02d", int(r*10.))));
        TGraphErrors *fracratio = new TGraphErrors;
        int np = 0;
        for(auto b : ROOT::TSeqI(0, hsparse->GetAxis(0)->GetNbins())) {
            hsparse->GetAxis(0)->SetRange(b+1, b+1);
            std::unique_ptr<TH1> clusterspec(hsparse->Projection(5)); 
            double all = clusterspec->Integral();
            if(!all) continue;
            double highe = clusterspec->Integral(clusterspec->GetXaxis()->FindBin(160.), clusterspec->GetXaxis()->FindBin(200.));
            fracratio->SetPoint(np, hsparse->GetAxis(0)->GetBinCenter(b+1), highe/all);
            fracratio->SetPointError(np, hsparse->GetAxis(0)->GetBinWidth(b+1)/2., highe/all * TMath::Sqrt(1./highe + 1./all));
            np++;
        }
        graphs[r] = fracratio;
    }

    auto plot = new ROOT6tools::TSavableCanvas("fracHighZ", "Fraction of high-z clusters", 800, 600);
    plot->cd();
    (new ROOT6tools::TAxisFrame("FrameRatio", "E_{cl} GeV/c", "Fract. clusters(E > 160 GeV/c)", 0., 200., 0., 0.1))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.15, 0.65, 0.3, 0.89);
    leg->Draw();
    std::map<double, Style> styles = {{0.2, {kRed, 24}}, {0.3, {kGreen+2, 25}}, {0.4, {kBlue, 26}}, {0.5, {kViolet, 27}}};
    for(auto r : graphs) {
        styles[r.first].SetStyle<TGraphErrors>(*r.second);
        r.second->Draw("epsame");
        leg->AddEntry(r.second, Form("R=%.1f", r.first), "lep");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}