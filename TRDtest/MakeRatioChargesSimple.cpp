#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"
#include "../struct/GraphicsPad.cxx"
#include "../unfolding/binnings/binninghelper.cpp"

struct Spectra {
	TH1 			*fPositive;
	TH1			*fNegative;
	TH1			*fRatioPositiveNegative;
};

TH1 *makeRebinned(TH1 *hist) {
        auto binhandler = binninghelper(0.15, {{0.3, 0.05}, {0.99999, 0.1}, {3., 0.2}, {10., 0.5}, {20.,1.}, {38, 2.}, {50., 4.}, {80., 10.}, {200., 20.}, {320, 40.}});
        auto newbinning = binhandler.CreateCombinedBinning();
        for(auto b : newbinning) std::cout << b << ", ";
        std::cout << std::endl;
        auto rebinned = hist->Rebin(newbinning.size()-1., Form("%srebinned", hist->GetName()), newbinning.data());
        return rebinned;
}

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
        TH1 *hneg = makeRebinned(htracks->Projection(0)); 
        neg->SetName(Form("hneg%s", trigger.data()));
	    hneg->SetDirectory(nullptr);
	    hneg->Scale(1./norm);
	    hneg->Scale(1., "width");
        htracks->GetAxis(6)->SetRange(2,2);
        TH1* hpos = makeRebinned(htracks->Projection(0)); 
        hpos->SetName(Form("hpos%s", trigger.data()));
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

void MakeRatioChargesSimple(int etasign = 0, int tracktype = -1, double ptmax = 250){
    auto ratios = loadRatios("AnalysisResults.root", etasign, tracktype);

    std::string etatag = "all";
    if(etasign < 0) etatag = "negeta";
    else etatag = "poseta";
    std::string ttstring = "tracksall";
    if(tracktype == 4) ttstring = "tracksglobal";
    else if(tracktype == 5) ttstring = "trackscomplementary";
    auto plot = new ROOT6tools::TSavableCanvas(Form("ComparisonChargeRatios_ptmax%d_%s_%s", int(ptmax), etatag.data(), ttstring.data()), "Comparison charge ratios", 1200, 800);
    plot->Divide(3,2);

    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};
    Style ratiostyle{kBlue, 25};
    std::map<std::string, Style> stylesSpec = {{"pos", {kRed, 24}}, {"neg", {kBlue, 25}}};
    int icol(0);
    for(const auto &t : triggers){
	auto data = ratios.find(t)->second;
        plot->cd(icol+1);
        GraphicsPad specpad(gPad);
	    specpad.Logy();
	    specpad.Frame(Form("SpectraCharges%s", t.data()), "p_{t} (GeV/c)", "N(h^{+})/N(h^-})", 0., ptmax, 1e-10, 100.);
        specpad.Label(0.15, 0.8, 0.5, 0.89, Form("Trigger: %s", t.data()));
	    if(icol == 0) specpad.Legend(0.5, 0.6, 0.89, 0.89);
	    specpad.Draw<TH1>(data.fPositive, stylesSpec["pos"], "h+");
	    specpad.Draw<TH1>(data.fNegative, stylesSpec["neg"], "h-");
        plot->cd(icol+4);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("RatioCharges%s", t.data()), "p_{t} (GeV/c)", "N(h^{+})/N(h^-})", 0., ptmax, 0., 3.);
        ratiopad.Label(0.15, 0.15, 0.5, 0.22, Form("Trigger: %s", t.data()));
        ratiopad.Draw<TH1>(data.fRatioPositiveNegative, ratiostyle);
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}
