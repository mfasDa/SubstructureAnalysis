void extractMaxLumiFromCorrectedOutput(const char *filename = "correctedSVD_poor_tc200.root"){
    const double kMBtoPB = 1e9,
                 kXSecInPB = 57.8 * kMBtoPB;
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    reader->cd("R02/rawlevel");
    auto histeventcounter = static_cast<TH1 *>(gDirectory->Get("hNorm")),
         histcentnotrdcorr = static_cast<TH1 *>(gDirectory->Get("hCENTNOTRDcorrection"));
    auto eventcount = histeventcounter->GetBinContent(1),
         centnotrdcorr = histcentnotrdcorr->GetBinContent(1);
    auto lumi = eventcount * centnotrdcorr / kXSecInPB;
    std::cout << "Extracted luminosity Lint = " << std::setprecision(3) << lumi << " pb-1" << std::endl; 
}