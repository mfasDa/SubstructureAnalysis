#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"

struct spectre{
    TH1 *fSpectreR02;
    TH1 *fSpectreR04;
};  

spectre readCharged(const std::string_view filecharged) {
    std::unique_ptr<TFile> reader(TFile::Open(filecharged.data(), "READ"));
    auto h2 = static_cast<TH1 *>(reader->Get("JetPt02")),
         h4 = static_cast<TH1 *>(reader->Get("JetPt04"));
    h2->SetDirectory(nullptr);
    h4->SetDirectory(nullptr);
    return {h2, h4};
}

TH1 *readFullFile(const std::string_view filefull, double radius) {
    std::unique_ptr<TFile> reader(TFile::Open(filefull.data(), "READ"));
    auto hist = static_cast<TH1 *>(reader->Get("reference"));
    hist->SetDirectory(nullptr);
    const double kSizeEmcalPhi = 1.88,
                 kSizeEmcalEta = 1.4;
    double acceptance = (kSizeEmcalPhi - 2 * radius) * (kSizeEmcalEta - 2 * radius) / (TMath::TwoPi());
    double crosssection = 57.8;
    double epsilon_vtx = 0.8228; // for the moment hard coded, for future analyses determined automatically from the output
    hist->Scale(crosssection*epsilon_vtx/acceptance);
    return hist;
}

spectre readFull(const std::string_view inputdir){
    return {readFullFile(Form("%s/Systematics1DPt_R02.root", inputdir.data()), 0.2), readFullFile(Form("%s/Systematics1DPt_R04.root", inputdir.data()), 0.4)};
}

spectre readTheoryCharged(const std::string_view filename) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto hist02 = static_cast<TH1 *>(reader->Get("hChJetR02")),
         hist04 = static_cast<TH1 *>(reader->Get("hChJetR04"));
    hist02->SetDirectory(nullptr);
    hist04->SetDirectory(nullptr);
    return {hist02, hist04};
}

TH1 *readTheoryFullFile(const std::string_view inputfile){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    auto hist = static_cast<TH1 *>(reader->Get("jetptspectrum"));
    hist->SetDirectory(nullptr);
    return hist;
}

spectre readTheoryFull(const std::string_view inputdir) {
    return {readTheoryFullFile(Form("%s/spectrumPOWHEG_R02.root", inputdir.data())), readTheoryFullFile(Form("%s/spectrumPOWHEG_R04.root", inputdir.data()))};
}

std::vector<double> makeSpectreOverlapBinning(const TH1 *charged, const TH1 *neutral){
    std::vector<double> binning;
    for(auto b : ROOT::TSeqI(0, charged->GetNbinsX())){
        double binlow = charged->GetXaxis()->GetBinLowEdge(b+1);
        if(binlow < 30) continue;
        if(binlow < neutral->GetXaxis()->GetBinLowEdge(1)) continue;
        if(binlow > neutral->GetXaxis()->GetBinUpEdge(neutral->GetXaxis()->GetNbins())) break;
        binning.emplace_back(binlow);
    }
    binning.emplace_back(charged->GetXaxis()->GetBinUpEdge(charged->GetXaxis()->GetNbins()));
    return binning;
}

TH1 *truncate(const TH1 *input, const std::vector<double> binning){
    auto truncated = new TH1D(Form("%sTruncated", input->GetName()), input->GetTitle(), binning.size()-1, binning.data());
    truncated->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, truncated->GetNbinsX())){
        auto binorig = input->GetXaxis()->FindBin(truncated->GetXaxis()->GetBinCenter(b+1));
        truncated->SetBinContent(b+1, input->GetBinContent(binorig));
        truncated->SetBinError(b+1, input->GetBinError(binorig));
    }
    return truncated;
}

void makeComparisonChargedNeutral(){
    auto spectreCharged  = readCharged("charged/InlusiveJetCSR0204.root"),
         spectreFull = readFull("full"),
         spectreChargedTheory = readTheoryCharged("theocharged/POWHEG_ChJet13TeV_CT14nlo.root"),
         spectreFullTheory = readTheoryFull("theofull");

    auto overlap = makeSpectreOverlapBinning(spectreCharged.fSpectreR02, spectreFull.fSpectreR04);
    auto spectreChargedTruncated = spectre{truncate(spectreCharged.fSpectreR02, overlap), truncate(spectreCharged.fSpectreR04, overlap)};
    auto spectreFullRebinned = spectre{rebinPtSpectrum(spectreFull.fSpectreR02, overlap), rebinPtSpectrum(spectreFull.fSpectreR04, overlap)};
    auto spectreFullTheoryRebinned = spectre{rebinPtSpectrum(spectreFullTheory.fSpectreR02, overlap), rebinPtSpectrum(spectreFullTheory.fSpectreR04, overlap)};
    auto spectreChargedTheoryRebinned = spectre{rebinPtSpectrum(spectreChargedTheory.fSpectreR02, overlap), rebinPtSpectrum(spectreChargedTheory.fSpectreR04, overlap)};

    auto plot = new ROOT6tools::TSavableCanvas("chargedFullComparison", "Comparison charged full jets", 1200, 1000);
    plot->Divide(2,2);

    plot->cd(1);
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame("specframeR02","p_{t} (GeV/c)", "d#sigma(R=0.2)/dp_{t}d#eta", 0., 120., 1e-9, 10))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.65, 0.7, 0.89, 0.89);
    leg->Draw();
    Style{kRed, 24}.SetStyle<TH1>(*spectreChargedTruncated.fSpectreR02);
    Style{kBlue, 25}.SetStyle<TH1>(*spectreFullRebinned.fSpectreR02);
    Style{kGreen+2, 26}.SetStyle<TH1>(*spectreChargedTheoryRebinned.fSpectreR02);
    Style{kViolet, 27}.SetStyle<TH1>(*spectreFullTheoryRebinned.fSpectreR02);
    spectreChargedTruncated.fSpectreR02->Draw("epsame");
    spectreFullRebinned.fSpectreR02->Draw("epsame");
    spectreChargedTheoryRebinned.fSpectreR02->Draw("epsame");
    spectreFullTheoryRebinned.fSpectreR02->Draw("epsame");
    leg->AddEntry(spectreChargedTruncated.fSpectreR02, "Charged, Truncated", "lep");
    leg->AddEntry(spectreFullRebinned.fSpectreR02, "Full, Rebinned", "lep");
    leg->AddEntry(spectreChargedTheoryRebinned.fSpectreR02, "POWHEG+PYTHIA, Charged, Truncated", "lep");
    leg->AddEntry(spectreFullTheoryRebinned.fSpectreR02, "POWHEG+PYTHIA, Full, Rebinned", "lep");

    plot->cd(2);
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame("specframeR04","p_{t} (GeV/c)", "d#sigma(R=0.4)/dp_{t}d#eta", 0., 120., 1e-9, 10))->Draw("axis");
    Style{kRed, 24}.SetStyle<TH1>(*spectreChargedTruncated.fSpectreR04);
    Style{kBlue, 25}.SetStyle<TH1>(*spectreFullRebinned.fSpectreR04);
    Style{kGreen+2, 26}.SetStyle<TH1>(*spectreChargedTheoryRebinned.fSpectreR04);
    Style{kViolet, 27}.SetStyle<TH1>(*spectreFullTheoryRebinned.fSpectreR04);
    spectreChargedTruncated.fSpectreR04->Draw("epsame");
    spectreFullRebinned.fSpectreR04->Draw("epsame");
    spectreChargedTheoryRebinned.fSpectreR04->Draw("epsame");
    spectreFullTheoryRebinned.fSpectreR04->Draw("epsame");

    plot->cd(3);
    (new ROOT6tools::TAxisFrame("ratioframeR02", "p_{t} (GeV/c)", "Ratio Full / Charged R=0.2", 0., 120., 0., 10.))->Draw("axis");
    auto legratio = new ROOT6tools::TDefaultLegend(0.2, 0.7, 0.5, 0.89);
    legratio->Draw();
    auto ratioR02 = histcopy(spectreFullRebinned.fSpectreR02);
    ratioR02->SetDirectory(nullptr);
    ratioR02->Divide(spectreChargedTruncated.fSpectreR02);
    Style{kBlack, 20}.SetStyle(*ratioR02);
    ratioR02->Draw("epsame");
    auto ratioR02Theory = histcopy(spectreFullTheoryRebinned.fSpectreR02);
    ratioR02Theory->SetDirectory(nullptr);
    ratioR02Theory->Divide(spectreChargedTheoryRebinned.fSpectreR02);
    Style{kRed, 24}.SetStyle<TH1>(*ratioR02Theory);
    ratioR02Theory->Draw("epsame");
    legratio->AddEntry(ratioR02, "Data", "lep");
    legratio->AddEntry(ratioR02Theory, "POWHEG+PYTHIA", "lep");

    plot->cd(4);
    (new ROOT6tools::TAxisFrame("ratioframeR04", "p_{t} (GeV/c)", "Ratio Full / Charged R=0.4", 0., 120., 0., 10.))->Draw("axis");
    auto ratioR04 = histcopy(spectreFullRebinned.fSpectreR04);
    ratioR04->SetDirectory(nullptr);
    ratioR04->Divide(spectreChargedTruncated.fSpectreR04);
    Style{kBlack, 20}.SetStyle(*ratioR04);
    ratioR04->Draw("epsame");
    auto ratioR04Theory = histcopy(spectreFullTheoryRebinned.fSpectreR04);
    ratioR04Theory->SetDirectory(nullptr);
    ratioR04Theory->Divide(spectreChargedTheoryRebinned.fSpectreR04);
    Style{kRed, 24}.SetStyle<TH1>(*ratioR04Theory);
    ratioR04Theory->Draw("epsame");

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());

}