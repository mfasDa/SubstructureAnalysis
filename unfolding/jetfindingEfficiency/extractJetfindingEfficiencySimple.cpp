#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/Rebinner.cxx"
#include "../binnings/binningPt1D.C"

std::map<double, TH1 *> readEfficiencies(const std::string_view inputfile, int ultraoption, const std::string_view sysvar){
    std::map<double, TH1 *> result;
    std::vector<double> binning;
    switch(ultraoption) {
    case 240: binning = getJetPtBinningNonLinTrueUltra240(); break;
    case 320: binning = getJetPtBinningNonLinTrueUltra300(); break;
    //default: binning = getJetPtBinningNonLinTrueLargeFineLow(); break;
    default: binning = getJetPtBinningNonLinTruePoor(); break;
    };
    Rebinner rebinner(binning);
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));   
    for(auto r = 0.2; r <= 0.6; r+=0.1) {
        std::stringstream dirname;
        dirname << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << int(r*10.) <<  "_INT7_" << sysvar;
        reader->cd(dirname.str().data());
        auto histos = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto hresponse = static_cast<TH2 *>(histos->FindObject("hJetResponseFine"));
        auto alljets = rebinner(static_cast<TH1 *>(histos->FindObject("hJetSpectrumPartAll")));
        auto efficiency = rebinner(hresponse->ProjectionY());
        efficiency->Divide(efficiency, alljets, 1., 1., "b");
        efficiency->SetDirectory(nullptr);
        result[r] = efficiency;
    }    
    return result;
}

void extractJetfindingEfficiencySimple(const std::string_view inputfile, int ultraoption, const std::string_view sysvar){
    auto efficiencies = readEfficiencies(inputfile, ultraoption, sysvar);

    auto plot = new ROOT6tools::TSavableCanvas(Form("JetFindingEff_ultra%d_%s", ultraoption, sysvar.data()), "Jet finding efficiency", 800, 600); 
    GraphicsPad effpad(plot);
    effpad.Frame("effframe", "p_{t,part} (GeV/c)", "Jet finding efficiency", 0., 350, 0., 1.1);
    effpad.Legend(0.7, 0.15, 0.89, 0.5);

    std::array<Color_t, 5> colors = {kRed, kBlue, kGreen, kViolet, kOrange};
    Style_t firststyle = 24; int icase = 0;
    for(const auto &[r, eff] : efficiencies){
        Style effstyle{colors[icase], Style_t(firststyle+icase)};
        effpad.Draw<TH1>(eff, effstyle, Form("R=%.1f", r));
        icase++;
    } 

    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}