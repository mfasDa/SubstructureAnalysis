#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/substructuretree.C"

int extractR(const std::string_view filename){
    auto tokens = tokenize(std::string(filename), '_');
    return std::stoi(tokens[2].substr(1));
}

void makePlotDeltaPtSlice(const std::string_view inputfile){
    auto radius = extractR(inputfile);
    auto plot = new ROOT6tools::TSavableCanvas(Form("SliceJES_R%02d", radius), Form("Slice JES R=%.1f", float(radius)/10.), 1200, 1000);
    plot->Divide(4,3);

    TH2* hdiff(nullptr);
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        hdiff = static_cast<TH2 *>(reader->Get("ptdiff_allptsim"));
        hdiff->SetDirectory(nullptr);
    }

    std::vector<std::pair<double, double>> ranges = {{20., 25.}, {25., 30.}, {30., 35.}, {35., 40.}, 
                                                     {40., 50.}, {50., 60.}, {60., 70.}, {70., 80.},
                                                     {80., 100.}, {100., 120.}, {120., 160}, {160., 200.}};
    int ipad = 1;
    for(auto r : ranges){
        auto hist = hdiff->ProjectionY(Form("jes_%d_%d", int(r.first), int(r.second)), hdiff->GetXaxis()->FindBin(r.first + 0.5), hdiff->GetXaxis()->FindBin(r.second - 0.5));
        hist->Scale(1./hist->Integral());
        hist->Rebin(4);
        Style{kBlue, 24}.SetStyle<TH1>(*hist);
        plot->cd(ipad);
        gPad->SetLeftMargin(0.15);
        gPad->SetRightMargin(0.05);
        (new ROOT6tools::TAxisFrame(Form("difframe_%d_%d", int(r.first), int(r.second)), "<(p_{t,det} - p_{t,part})/p_{t, part}>", Form("Prob/Bin(%.2f)", hist->GetXaxis()->GetBinWidth(1)), -1., 0.4, 0., 0.14))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.25, 0.8, 0.75, 0.87, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", r.first, r.second)))->Draw();
        if(ipad == 1){
            (new ROOT6tools::TNDCLabel(0.2, 0.73, 0.55, 0.79, Form("Full jets, R=%.1f", float(radius)/10.)))->Draw();
        } 
        hist->Draw("epsame");
        auto zeroline = new TLine(0., 0.01, 0., 0.1);
        zeroline->SetLineColor(kBlack);
        zeroline->SetLineStyle(2);
        zeroline->Draw("epsame");
        ipad++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
} 