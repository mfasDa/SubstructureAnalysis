#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/root.C"
#include "../helpers/math.C"
#include "../helpers/graphics.C"
#include "../unfolding/binnings/binningZg.C"

TH2 *getPtHardWeightedZg( int pthardbin, double r){
    std::stringstream filename;
    filename << "mfaselr" << int(r*10.) << "pt" << pthardbin << ".root";
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
    auto binningZg = getZgBinningFine(),
         binningPtMB = getMinBiasPtBinningPart(),
         binningPtEJ2 = getEJ2PtBinningPart(),
         binningPtEJ1 = getEJ1PtBinningPart();
    // Make combined pt binning
    std::vector<double> binningPtCombined;
    for(auto b : binningPtMB) {
        if(b >= 30. && b < 80.) binningPtCombined.push_back(b);
    }
    for(auto b : binningPtEJ2) {
        if(b >= 80. && b < 120.) binningPtCombined.push_back(b);
    }
    for(auto b : binningPtEJ1) {
        if(b >= 120. && b <= 200.) binningPtCombined.push_back(b);
    }
    ROOT::RDataFrame specframe(Form("JetShapesMC_Jet_AKTFullR%02d0_mctracks_pT0000_E_scheme_TreeMC_GenShapes_NoSub_Incl", int(r*10.)), filename.str());
    auto spectrum = specframe.Histo2D({"zgdist", "Jet p_{t} spectrum", static_cast<int>(binningZg.size())-1, binningZg.data(), static_cast<int>(binningPtCombined.size())-1, binningPtCombined.data()}, "zg", "ptJet");
    spectrum->Scale(weightPythia);
    return static_cast<TH2 *>(histcopy(spectrum.GetPtr()));
}

void extractZgSimV1(double r){
    TH2 *sum(nullptr);
    for(auto b : ROOT::TSeqI(1, 11)){
        auto binhist = getPtHardWeightedZg(b, r);
        if(sum) sum->Add(binhist);
        else sum = static_cast<TH2*>(histcopy(binhist));
    }
    sum->SetName("zgdistSum");

    std::stringstream outfilename;
    outfilename << "zgdists_Perugia11_R" << std::setw(2) << std::setfill('0') << int(r*10.) << ".root";
    std::unique_ptr<TFile> writer(TFile::Open(outfilename.str().data(), "RECREATE"));
    writer->cd();
    sum->Write();
    for(auto b : ROOT::TSeqI(0, sum->GetYaxis()->GetNbins())) {
        std::stringstream histname, histtitle;
        histname << "zgpt_" << static_cast<int>(sum->GetYaxis()->GetBinLowEdge(b+1)) << "_" << static_cast<int>(sum->GetYaxis()->GetBinUpEdge(b+1));
        histtitle << "Zg for " << std::fixed << std::setprecision(2) << sum->GetYaxis()->GetBinLowEdge(b+1) << " GeV/c < p_{t} < " << sum->GetYaxis()->GetBinUpEdge(b+1) << " GeV/c";
        auto projectionBin = sum->ProjectionX(histname.str().data(), b+1, b+1);
        projectionBin->SetTitle(histtitle.str().data());
        projectionBin->Scale(1./projectionBin->Integral());
        normalizeBinWidth(projectionBin);
        projectionBin->Write();
    }
}