#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../struct/GraphicsPad.cxx"

std::map<std::string, TH1 *> loadRatios(const std::string_view filename){
    std::map<std::string, TH1 *> data;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->ls();
    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};
    for(const auto &t : triggers){
        std::stringstream dirname;
        dirname << "AliEmcalTrackingQATask_";
        if(t != "INT7") dirname << t << "_";
        dirname << "histos";
	std::cout << "Reading " << dirname.str() << std::endl;
        auto histlist = dynamic_cast<TList *>(reader->Get(dirname.str().data()));
	if(!histlist) std::cout << dirname.str().data() << " not found" << std::endl;
        auto htracks = dynamic_cast<THnSparse *>(histlist->FindObject("fTracks"));
	htracks->Sumw2();
        // negative charge
        htracks->GetAxis(6)->SetRange(1,1);
        std::unique_ptr<TH1> hneg(htracks->Projection(0)); 
        htracks->GetAxis(6)->SetRange(2,2);
        std::unique_ptr<TH1> hpos(htracks->Projection(0)); 

        TH1 *ratioC = static_cast<TH1 *>(hpos->Clone(Form("ratioPosNeg_%s", t.data())));
        ratioC->SetDirectory(nullptr);
        ratioC->Divide(hneg.get());
        data[t] = ratioC;
    }
    return data;
}

void MakeRatioCharges(){
    auto ratiosWithTRD = loadRatios("withTRD/data/AnalysisResults.root"),
         ratiosWithoutTRD = loadRatios("withoutTRD/data/LHC17o/AnalysisResults.root");

    auto plot = new ROOT6tools::TSavableCanvas("ComparisonChargeRatios", "Comparison charge ratios", 1200, 700);
    plot->Divide(3,1);

    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};
    std::map<std::string, Style> styles = {{"withTRD", {kRed, 24}}, {"withoutTRD", {kBlue, 25}}};
    int icol(1);
    for(const auto &t : triggers){
        plot->cd(icol);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("RatioCharges%s", t.data()), "p_{t} (GeV/c)", "N(h^{+})/N(h^-})", 0., 250., 0.5, 1.5);
        ratiopad.Label(0.15, 0.15, 0.5, 0.22, Form("Trigger: %s", t.data()));
        if(icol == 0) ratiopad.Legend(0.15, 0.7, 0.5, 0.89);
        ratiopad.Draw<TH1>(ratiosWithTRD.find(t)->second, styles["withTRD"], "With TRD");
        ratiopad.Draw<TH1>(ratiosWithoutTRD.find(t)->second, styles["withoutTRD"], "Without TRD");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}
