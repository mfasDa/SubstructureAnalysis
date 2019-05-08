#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../struct/JetSpectrumReader.cxx"
#include "../../struct/GraphicsPad.cxx"

void plotStatRelUnfolded(const std::string_view inputfile){
    std::vector<std::string> spectra = {"normalized_reg4"};
    JetSpectrumReader reader(inputfile, spectra);
    
    auto plot = new ROOT6tools::TSavableCanvas("relStatErrorUnfolded", "Relative statistical error of the unfolded spectrum", 1200, 800);
    plot->Divide(3,2);

    Style statstyle{kBlue, 20};

    int ipad(1);
    for(auto r : reader.GetJetSpectra().GetJetRadii()){
        plot->cd(ipad++);
        std::string rstring(Form("R%02d", int(r*10.)));
        GraphicsPad errorpad(gPad);
        errorpad.Frame(Form("errorpad%s", rstring.data()), "p_{t} (GeV/c)", "rel. stat. error", 0., 350., 0., 0.1);
        errorpad.Margins(0.12, 0.05, -1, 0.05);
        errorpad.Label(0.2, 0.86, 0.4, 0.93, Form("R=%.1f", r));
        auto spec = reader.GetJetSpectrum(r, spectra[0]);
        auto relstat = (TH1 *)spec->Clone(Form("relStat%s",rstring.data()));
        for(auto b : ROOT::TSeqI(0, relstat->GetXaxis()->GetNbins())){
            relstat->SetBinContent(b+1, relstat->GetBinError(b+1)/relstat->GetBinContent(b+1));
            relstat->SetBinError(b+1, 0.);
        }
        relstat->SetLineColor(kBlue);
        relstat->Draw("same");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}