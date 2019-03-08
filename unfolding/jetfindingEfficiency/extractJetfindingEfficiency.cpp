#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

#include "../../helpers/graphics.C"
#include "../binnings/binningPt1D.C"

TH1 *makeJetfindingEfficiency(TFile &reader, const std::string_view rstring, bool finebinning){
    auto binning = finebinning ? getJetPtBinningNonLinTrueLargeFine() : getJetPtBinningNonLinTrueLarge();
    reader.cd(Form("EnergyScaleResults_FullJet_%s_INT7", rstring.data()));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    std::unique_ptr<THnSparse> hsparse(static_cast<THnSparse*>(histlist->FindObject("hJetfindingEfficiency")));
    std::unique_ptr<TH1> halltmp(hsparse->Projection(0)),
                         hall(halltmp->Rebin(binning.size()-1, Form("All_%s", rstring.data()), binning.data()));
    hsparse->GetAxis(2)->SetRange(3,3);
    std::unique_ptr<TH1> hmatch(hsparse->Projection(0));
    TH1 *heff = hmatch->Rebin(binning.size()-1, Form("Efficiency_%s", rstring.data()), binning.data());
    heff->SetDirectory(nullptr);
    heff->Divide(heff, hall.get(), 1., 1., "b");
    hsparse->GetAxis(2)->SetRange(1,3);
    return heff;
}

void extractJetfindingEfficiency(const std::string_view inputfile = "AnalysisResults.root", bool finebinning = false){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));

    auto plot = new ROOT6tools::TSavableCanvas(Form("jetfindingeff%s", (finebinning ? "Fine" : "Standard")), "Jetfinding Efficiency", 800, 600);
    plot->cd();

    (new ROOT6tools::TAxisFrame("effframe", "p_{t, part} (GeV/c)", " #epsilon_{jet finding}", 0., 300., 0., 1.1))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.14, 0.89, 0.5);
    leg->Draw();

    std::array<Color_t, 6> colors = {kRed, kBlue, kGreen, kViolet, kOrange, kMagenta};
    std::array<Style_t, 6> markers = {24, 25, 26, 27, 28, 29};

    int ihist = 0;
    for(auto rval : ROOT::TSeqI(2,7)) {
        std::string rstring(Form("R%02d", rval));
        auto effhist = makeJetfindingEfficiency(*reader, rstring, finebinning);
        Style{colors[ihist], markers[ihist]}.SetStyle<TH1>(*effhist);
        effhist->Draw("epsame");
        leg->AddEntry(effhist, Form("R=%.1f", double(rval)/10.), "lep");
        ihist++;
    }

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}