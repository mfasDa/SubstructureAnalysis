#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../struct/GraphicsPad.cxx"

struct Spectra {
	TH1 			*fPositive;
	TH1			*fNegative;
	TH1			*fRatioPositiveNegative;
};

std::map<std::string, Spectra> loadRatios(const std::string_view filename, int etasign, int tracktype){
    std::map<std::string, Spectra> data;
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
	auto norm = dynamic_cast<TH1 *>(histlist->FindObject("fHistEventCount"))->GetBinContent(1);
	htracks->Sumw2();
	if(etasign != 0){
            if(etasign < 0) htracks->GetAxis(1)->SetRangeUser(-0.799, -0.01);
	    else htracks->GetAxis(1)->SetRangeUser(0.01, 0.799);
	}
	if(tracktype >-1) {
		htracks->GetAxis(4)->SetRange(tracktype+1, tracktype+1);
	}
        // negative charge
        htracks->GetAxis(6)->SetRange(1,1);
        TH1 *hneg(htracks->Projection(0)); 
	hneg->SetDirectory(nullptr);
	hneg->Scale(1./norm);
	hneg->Scale(1., "width");
        htracks->GetAxis(6)->SetRange(2,2);
        TH1* hpos(htracks->Projection(0)); 
	hpos->SetDirectory(nullptr);
	hpos->Scale(1./norm);
	hpos->Scale(1., "width");

        TH1 *ratioC = static_cast<TH1 *>(hpos->Clone(Form("ratioPosNeg_%s", t.data())));
        ratioC->SetDirectory(nullptr);
        ratioC->Divide(hneg);
        data[t] = {hpos, hneg, ratioC};
    }
    return data;
}

void MakeRatioCharges(int etasign = 0, int tracktype = -1, double ptmax = 250){
    auto ratiosWithTRD = loadRatios("withTRD/data/AnalysisResults.root", etasign, tracktype),
         ratiosWithoutTRD = loadRatios("withoutTRD/data/LHC17o/AnalysisResults.root", etasign, tracktype);

    std::string etatag = "all";
    if(etasign < 0) etatag = "negeta";
    else etatag = "poseta";
    std::string ttstring = "tracksall";
    if(tracktype == 4) ttstring = "tracksglobal";
    else if(tracktype == 5) ttstring = "trackscomplementary";
    auto plot = new ROOT6tools::TSavableCanvas(Form("ComparisonChargeRatios_ptmax%d_%s_%s", int(ptmax), etatag.data(), ttstring.data()), "Comparison charge ratios", 1200, 800);
    plot->Divide(3,2);

    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};
    std::map<std::string, Style> styles = {{"withTRD", {kRed, 24}}, {"withoutTRD", {kBlue, 25}}};
    std::map<std::string, Style> stylesSpec = {{"poswith", {kRed, 24}}, {"poswithout", {kRed, 20}}, {"negwith", {kBlue, 25}}, {"negwithout", {kBlue, 21}}};
    int icol(0);
    for(const auto &t : triggers){
	auto datawith = ratiosWithTRD.find(t)->second,
	     datawithout = ratiosWithoutTRD.find(t)->second;
        plot->cd(icol+1);
        GraphicsPad specpad(gPad);
	specpad.Logy();
	specpad.Frame(Form("SpectraCharges%s", t.data()), "p_{t} (GeV/c)", "N(h^{+})/N(h^-})", 0., ptmax, 1e-10, 100.);
        specpad.Label(0.15, 0.15, 0.5, 0.22, Form("Trigger: %s", t.data()));
	if(icol == 0) specpad.Legend(0.5, 0.6, 0.89, 0.89);
	specpad.Draw<TH1>(datawith.fPositive, stylesSpec["poswith"], "with TRD, h+");
	specpad.Draw<TH1>(datawith.fNegative, stylesSpec["negwith"], "with TRD, h-");
	specpad.Draw<TH1>(datawithout.fPositive, stylesSpec["poswithout"], "without TRD, h+");
	specpad.Draw<TH1>(datawithout.fNegative, stylesSpec["negwithout"], "without TRD, h-");
        plot->cd(icol+4);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("RatioCharges%s", t.data()), "p_{t} (GeV/c)", "N(h^{+})/N(h^-})", 0., ptmax, 0.5, 1.5);
        ratiopad.Label(0.15, 0.15, 0.5, 0.22, Form("Trigger: %s", t.data()));
        if(icol == 0) ratiopad.Legend(0.15, 0.7, 0.5, 0.89);
        ratiopad.Draw<TH1>(datawith.fRatioPositiveNegative, styles["withTRD"], "With TRD");
        ratiopad.Draw<TH1>(datawithout.fRatioPositiveNegative, styles["withoutTRD"], "Without TRD");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}
