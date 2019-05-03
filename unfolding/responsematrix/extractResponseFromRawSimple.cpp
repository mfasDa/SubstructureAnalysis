#include "../binnings/binningPt1D.C"
#include "../../helpers/unfolding.C"
#include "../../helpers/math.C"
#include "../../meta/stl.C"
#include "../../meta/root.C"

void extractResponseFromRawSimple(const std::string_view filename="AnalysisResults.root", const std::string_view sysvar = "tc200"){
    auto binningpart = getJetPtBinningNonLinTrueLargeFineLow(),
         binningdet = getJetPtBinningNonLinSmearLargeFineLow();
    std::unique_ptr<TFile> reader(TFile::Open("AnalysisResults.root", "READ")),
                           writer(TFile::Open("Responsematrix.root", "RECREATE"));
    for(auto r = 0.2; r < 0.6; r+=0.1) {
        reader->cd(Form("EnergyScaleResults_FullJet_R%02d_INT7_%s", int(r*10.), sysvar.data()));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto responsefine = static_cast<TH2 *>(histlist->FindObject("hJetResponseFine"));
        responsefine->SetName(Form("Rawresponse_R%02d", int(r*10.)));
        auto rebinnedresponse  = makeRebinned2D(responsefine, binningdet, binningpart);
        rebinnedresponse->SetName(Form("%s_standard", rebinnedresponse->GetName()));
        writer->mkdir(Form("R%02d", int(r*10.)));
        writer->cd(Form("R%02d", int(r*10.)));
        gDirectory->mkdir("response");
        gDirectory->cd("response");
        responsefine->Write();
        rebinnedresponse->Write();
    }
}