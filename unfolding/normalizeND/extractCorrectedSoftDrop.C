#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

void extractCorrectedSoftDrop(const char *filename = "UnfoldedSD.root", bool withEfficiencyCorrection = true, int defaultiteration = 6){
    const double ptmin = 15., ptmax = 200.;
    std::stringstream outfilename;
    outfilename << "CorrectedSoftDrop";
    if(!withEfficiencyCorrection) {
        outfilename << "_noEffCorr";
    }
    outfilename << ".root";
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ")),
                           writer(TFile::Open(outfilename.str().data(), "RECREATE"));
    std::vector<std::string> observables;
    for(auto key : TRangeDynCast<TKey>(reader->GetListOfKeys())) observables.push_back(key->GetName());

    for(auto obs : observables) {
        writer->mkdir(obs.data());
        writer->cd(obs.data());
        auto writebasedir = gDirectory;
        reader->cd(obs.data());
        auto basedir = gDirectory;
        for(auto R : ROOT::TSeqI(2, 7)) {
            std::vector<TH1 *> dists;
            std::string rstring = Form("R%02d", R),
                        rtitle = Form("R = %.1f", double(R)/10.);
            basedir->cd(rstring.data());
            gDirectory->cd(Form("Iter%d", defaultiteration));
            std::stringstream inputhist;
            inputhist << "corrected";
            if(!withEfficiencyCorrection) {
                inputhist << "NoJetFindingEff";
            }
            inputhist << "Iter" << defaultiteration << "_" << obs << "_" << rstring;
            auto correctedhist = static_cast<TH2 *>(gDirectory->Get(inputhist.str().data()));
            for(auto iptb : ROOT::TSeqI(0, correctedhist->GetYaxis()->GetNbins())) {
                auto ptlow = correctedhist->GetYaxis()->GetBinLowEdge(iptb+1),
                     pthigh = correctedhist->GetYaxis()->GetBinUpEdge(iptb+1),
                     ptcent = correctedhist->GetYaxis()->GetBinCenter(iptb+1);
                if(ptcent < ptmin || ptcent > ptmax) continue;
                std::cout << "Doing bin from " << ptlow << " GeV/c to " << pthigh << " GeV/c" << std::endl;
                auto projected = correctedhist->ProjectionX(Form("%s_%s_%d_%d", obs.data(), rstring.data(), int(ptlow), int(pthigh)), iptb+1, iptb+1);
                std::cout << "Got projection with name " << projected->GetName() << std::endl;
                projected->SetDirectory(nullptr);
                projected->Scale(1./projected->Integral());
                projected->Scale(1., "width"); 
                projected->SetTitle(Form("%s for jets with %s and %.1f GeV/c < p_{t} < %.1f GeV/c", obs.data(), rtitle.data(), ptlow, pthigh));
                dists.push_back(projected);
            }

            // write to file
            writebasedir->mkdir(rstring.data());
            writebasedir->cd(rstring.data());
            for(auto hist : dists) hist->Write();
        }
    }
}