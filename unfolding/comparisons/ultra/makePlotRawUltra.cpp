#include "../../../helpers/graphics.C"

std::map<double, TH1 *> readRawSpectra(const std::string_view filename){
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(double r = 0.2; r < 0.7; r+= 0.1) {
        std::cout << "Doing jet radius " << r << " ..." << std::endl;
        std::string rstring = Form("R%02d", int(r*10.));
        reader->cd(rstring.data());
        gDirectory->cd("rawlevel");
        auto norm = static_cast<TH1 *>(gDirectory->Get("hNorm"));
        auto spectrum = static_cast<TH1 *>(gDirectory->Get(Form("hraw_%s", rstring.data())));
        spectrum->SetDirectory(nullptr);
        spectrum->Scale(norm->GetBinContent(1)/57.8);
        // check that the errors are really sqrt(N)
        bool consistent = true;
        for(auto b : ROOT::TSeqI(0, spectrum->GetXaxis()->GetNbins())){
            if(spectrum->GetBinError(b+1) != TMath::Sqrt(spectrum->GetBinContent(b+1))) {
                std::cout << "Inconsistency in bin " << b+1 << "[" << spectrum->GetXaxis()->GetBinLowEdge(b+1) << "|" 
                         << spectrum->GetXaxis()->GetBinUpEdge(b+1) << "]: expected " << TMath::Sqrt(spectrum->GetBinContent(b+1)) 
                         << " (" << spectrum->GetBinContent(b+1) << "), observed " << spectrum->GetBinError(b+1) << std::endl;
                consistent = false;
            }
        }
        if(consistent) std::cout << "Errors are consistent" << std::endl;
        else std::cout << "Inconsistency in errors detected" << std::endl;
        spectrum->Scale(1., "width");
        result[r] = spectrum;
    }
    return result;
}

void makePlotRawUltra(const std::string_view inputfile  = "correctedSVD_ultra.root") {
    auto specdata = readRawSpectra(inputfile);

    std::map<double, Style> rstyles = {{0.2, {kRed, 24}}, {0.3, {kBlue, 25}}, 
                                       {0.4, {kGreen, 26}}, {0.5, {kOrange, 26}}, {0.6, {kViolet, 28}}};

    // Spectra
    auto specplot = new ROOT6tools::TSavableCanvas("rawYieldsWideScale", "Raw yield wide scale", 800, 600);
    gPad->SetLogy();
    (new ROOT6tools::TAxisFrame("specframe", "p_{t} (GeV/c)", "N_{jet}", 0., 300., 0.5, 1e9))->Draw("axis");
    auto leg = new ROOT6tools::TDefaultLegend(0.7, 0.5, 0.89, 0.89);
    leg->Draw();
    for(auto [r, spec] : specdata) {
        rstyles[r].SetStyle<TH1>(*spec);
        spec->Draw("epsame");
        leg->AddEntry(spec, Form("R=%.1f", r), "lep");
    }

    specplot->cd();
    specplot->Update();
    specplot->SaveCanvas(specplot->GetName());

    // relative statistical error
    auto errplot = new ROOT6tools::TSavableCanvas("relStatErrWideScale", "Rel. stat. error", 800, 600);
    (new ROOT6tools::TAxisFrame("errframe", "p_{t} (GeV/c)", "rel. stat error", 0., 300., 0., 0.2))->Draw("axis");
    leg = new ROOT6tools::TDefaultLegend(0.15, 0.5, 0.3, 0.89);
    leg->Draw();
    for(auto [r, spec] : specdata) {
        auto relstat = static_cast<TH1 *>(spec->Clone(Form("relStatErrR%02d", int(r*10.))));
        relstat->SetDirectory(nullptr);
        rstyles[r].SetStyle<TH1>(*relstat);
        for(auto b : ROOT::TSeqI(0, relstat->GetXaxis()->GetNbins())) {
            relstat->SetBinContent(b+1, relstat->GetBinError(b+1)/relstat->GetBinContent(b+1));
            relstat->SetBinError(b+1, 0.);
        }
        relstat->Draw("epsame");
        leg->AddEntry(relstat, Form("R=%.1f", r), "lep");
    }
    errplot->cd();
    errplot->Update();
    errplot->SaveCanvas(errplot->GetName());
}