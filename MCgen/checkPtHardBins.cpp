#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/root.C"
#include "../helpers/math.C"
#include "../helpers/graphics.C"
#include "../unfolding/binnings/binningPt1D.C"

TH1 *getPtHardWeightedSpectrum(bool fine, int pthardbin, double r){
    std::stringstream filename;
    filename << "markusr" << int(r*10.) << "pt" << pthardbin << ".root";
    double weightPythia;
    {
        std::cout << "reading spectrum from " << filename.str() << std::endl;
        std::unique_ptr<TFile> filereader(TFile::Open(filename.str().data(), "READ"));
        auto histlist = static_cast<TList *>(filereader->Get(Form("JetShapesMC_Jet_AKTFullR%02d0_mctracks_pT0000_E_scheme_HistosMC_GenShapes_NoSub_Incl", int(r*10.))));
        auto hTrials = static_cast<TH1*>(histlist->FindObject("fHistTrialsAfterSel"));
        auto hXsec = static_cast<TProfile*>(histlist->FindObject("fHistXsectionAfterSel"));
        auto xsection = hXsec->GetBinContent(pthardbin+1);
        auto nTrials  = hTrials->GetBinContent(pthardbin+1);
        weightPythia  = xsection / nTrials;
        std::cout << "found cross section " << weightPythia << std::endl;
    }
    std::vector<double> binningpart;
    if(fine) {
        for(auto b : ROOT::TSeqI(0, 301)) binningpart.emplace_back(b);
    } else {
        binningpart = getJetPtBinningNonLinTrueLarge();
    }
    ROOT::RDataFrame specframe(Form("JetShapesMC_Jet_AKTFullR%02d0_mctracks_pT0000_E_scheme_TreeMC_GenShapes_NoSub_Incl", int(r*10.)), filename.str());
    auto spectrum = specframe.Histo1D({"jetptspectrum", "Jet p_{t} spectrum", static_cast<int>(binningpart.size())-1, binningpart.data()}, "ptJet");
    normalizeBinWidth(spectrum.GetPtr());
    double acceptancecorr = 1.8 - 2 * r;
    spectrum->Scale(weightPythia/acceptancecorr);
    return histcopy(spectrum.GetPtr());
}

void checkPtHardBins(double r, bool fine = true){
    std::stringstream canvname;
    canvname << "ptHardComparison";
    if(fine) canvname << "Fine";
    canvname << "R" << std::setw(2) << std::setfill('0') << int(r*10.);
    auto plot = new ROOT6tools::TSavableCanvas(canvname.str().data(), Form("pthard comparison %f", r), 800, 600);
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame("compframe", "p_{t} (GeV/c)", "d#sigma/dp_{t}d#eta", 0., 350, 1e-10, 1))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.45, 0.6, 0.89, 0.89);
    leg->SetNColumns(2);
    leg->Draw();
    (new ROOT6tools::TNDCLabel(0.15, 0.8, 0.25, 0.89, Form("R=%.1f", r)))->Draw();
    
    std::array<Color_t, 10> colors = {kRed, kBlue, kGreen, kOrange, kViolet, kGray, kCyan, kMagenta, kTeal, kAzure};
    std::array<Style_t, 10> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};
    std::vector<TH1 *> binhists;
    TH1 *sum(nullptr);
    for(auto b : ROOT::TSeqI(1, 11)){
        auto binhist = getPtHardWeightedSpectrum(fine, b, r);
        binhist->SetName(Form("bin%d", b));
        binhists.push_back(binhist);
        Style{colors[b-1], markers[b-1]}.SetStyle<TH1>(*binhist);
        binhist->Draw("epsame");
        leg->AddEntry(binhist, Form("bin %d", b), "lep");
        if(sum) sum->Add(binhist);
        else sum = histcopy(binhist);
    }
    sum->SetName("jetptspectrum");
    Style{kBlack, 20}.SetStyle<TH1>(*sum);
    sum->Draw("epsame");
    leg->AddEntry(sum, "Sum", "lep");
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());

    std::stringstream outfile;
    outfile << "spectrum";
    if(fine) outfile << "Fine";
    outfile << "Perugia11_R" << std::setw(2) << std::setfill('0') << int(r*10) << ".root";
    std::unique_ptr<TFile> writer(TFile::Open(outfile.str().data(), "RECREATE"));
    writer->cd();
    sum->Write();
    for(auto b : binhists) b->Write();
}