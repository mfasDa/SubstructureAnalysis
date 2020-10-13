#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../helpers/math.C"
#include "../binnings/binningPt1D.C"

TH2 *getResponseMatrix(TFile &reader, double R, const std::string_view sysvar, int closurestatus = 0) {
    std::string closuretag;
    switch(closurestatus) {
    case 0: break;
    case 1: closuretag = "Closure"; break;
    case 2: closuretag = "NoClosure"; break;   
    };
    std::string responsematrixbase = "hJetResponseFine";
    std::stringstream responsematrixname;
    responsematrixname << responsematrixbase;
    if(closuretag.length()) responsematrixname <<  closuretag;
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << int(R*10.) << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto rawresponse = static_cast<TH2 *>(histlist->FindObject(responsematrixname.str().data()));
    rawresponse->SetDirectory(nullptr);
    rawresponse->SetNameTitle(Form("Rawresponse_R%02d", int(R*10.)), Form("Raw response R=%.1f", R));
    return rawresponse;
}

void extractKinematicEfficiency1D_SpectrumTask(const std::string_view filename = "AnalysisResults.root", const std::string_view sysvar = "tc200") {
    std::stringstream outputfile;
    outputfile << "correctedSVD_poor";
    if(sysvar.length()) {
        outputfile << "_" << sysvar;
    }
    outputfile << ".root";
    std::unique_ptr<TFile> mcreader(TFile::Open(filename.data(), "READ")),
                           writer(TFile::Open(outputfile.str().data(), "RECREATE"));
    auto binningpart = getJetPtBinningNonLinTruePoor(),
         binningdet = getJetPtBinningNonLinSmearPoor();
    for(double radius = 0.2; radius <= 0.6; radius += 0.1) {
        // Get the response matrix
        auto rawresponse = getResponseMatrix(*mcreader, radius, sysvar);
        rawresponse->SetName(Form("%s_fine", rawresponse->GetName()));
        rawresponse->Scale(40e6);   // undo scaling with the number of trials
        auto rebinnedresponse  = makeRebinned2D(rawresponse, binningdet, binningpart);
        rebinnedresponse->SetName(Form("%s_standard", rebinnedresponse->GetName()));
        std::unique_ptr<TH1> truefulltmp(rawresponse->ProjectionY()),
                             truetmp(rebinnedresponse->ProjectionY());
        auto truefull = truefulltmp->Rebin(binningpart.size() - 1, Form("truefull_R%02d", int(radius*10.)), binningpart.data());
        truefull->SetDirectory(nullptr);
        auto effkine = truetmp->Rebin(binningpart.size()-1, Form("effKine_R%02d", int(radius*10.)), binningpart.data());
        effkine->SetDirectory(nullptr);
        effkine->Divide(effkine, truefull, 1., 1., "b");
        
        writer->mkdir(Form("R%02d", int(radius*10)));
        writer->cd(Form("R%02d", int(radius*10)));
        auto basedir = gDirectory;
        basedir->mkdir("response");
        basedir->cd("response");
        rawresponse->Write();
        rebinnedresponse->Write();
        truefull->Write();
        effkine->Write();
    }
}