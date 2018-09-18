#include "../../meta/root.C"
#include "../../meta/root6tools.C"

TH2 *readFile(const std::string_view filename, int iter = 10) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd(Form("iteration%d", iter));
    TH2 *h2 = static_cast<TH2 *>(gDirectory->Get(Form("zg_unfolded_iter%d", iter)));
    h2->SetDirectory(nullptr);
    return h2;
}

TH2 *makeRatioStrongLoose(const std::string_view dirstrong, const std::string_view dirloose, double r, const std::string_view trigger) {
    std::stringstream filestrong, fileloose, rootfile;
    rootfile << "JetSubstructureTree_FullJets_R" << std::setw(2) << std::setfill('0') << int(r*10) << "_" << trigger << "_unfolded_zg.root";
    filestrong << dirstrong << "/" << rootfile.str();
    fileloose << dirloose << "/" << rootfile.str();
    TH2 *hstrong = readFile(filestrong.str()), *hloose = readFile(fileloose.str());
    hstrong->SetNameTitle(Form("ratioStrongLoose_R%02d_%s", int(r*10), trigger.data()), Form("Strong/Loose R=%.1f, %s", r, trigger.data()));
    hstrong->Sumw2();
    hloose->Sumw2();
    hstrong->Divide(hloose);
    delete hloose;
    return hstrong;
}


void ratioOutlierCuts(const std::string_view dirstrong, const std::string_view dirloose) {
    std::vector<double> jetradii = {0.2, 0.3, 0.4, 0.5};
    std::vector<std::string> triggers = {"INT7", "EJ2", "EJ1"};
    auto plot = new ROOT6tools::TSavableCanvas("ComparisonOutlierCuts", "Comparison outlier cuts", 1400, 1000);
    plot->Divide(jetradii.size(), triggers.size());
    int ipad = 1;
    for(auto t : triggers) {
        for(auto r : jetradii) {
            auto hist = makeRatioStrongLoose(dirstrong, dirloose, r, t);
            hist->SetXTitle("z_{g}");
            hist->SetYTitle("p_{t} (GeV/c)");
            plot->cd(ipad++);
            hist->Draw("colz");
        }
    }
    plot->cd();
    plot->Update();
}