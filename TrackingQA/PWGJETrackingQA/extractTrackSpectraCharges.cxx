#ifndef __CLING__
#include "../../meta/stl.C"
#include "../../meta/root.C"
#endif

struct HistCollection {
    TH1 *hAllCharge;
    TH1 *hNegCharge;
    TH1 *hPosCharge;
};

HistCollection extractSpectra(THnSparse *trackTHnSparse, int trackType, bool restrictEMCAL) {
    std::string trackTypeString,
                acceptanceString = restrictEMCAL ? "EMCAL" : "all";
    int trackTypeBin = -1;
    switch(trackType){ 
        case 0: {
            trackTypeString = "all";
            break;
        }
        case 1: {
            trackTypeString = "glob";
            trackTypeBin = 1;
            break;
        }
        case 2: {
            trackTypeString = "comp";
            trackTypeBin = 2;
            break;
        }
        default: break;
    };

    const int kEtaAxis = 1,
              kPhiAxis = 2,
              kTrackTypeAxis = 4,
              kChargeAxis = 6;

    double backupMinTrackAxis = trackTHnSparse->GetAxis(kTrackTypeAxis)->GetXmin(),
           backupMaxTrackAxis = trackTHnSparse->GetAxis(kTrackTypeAxis)->GetXmax(),
           backupMinEtaAxis = trackTHnSparse->GetAxis(kEtaAxis)->GetXmin(),
           backupMaxEtaAxis = trackTHnSparse->GetAxis(kEtaAxis)->GetXmax(),
           backupMinPhiAxis = trackTHnSparse->GetAxis(kPhiAxis)->GetXmin(),
           backupMaxPhiAxis = trackTHnSparse->GetAxis(kPhiAxis)->GetXmax(),
           backupMinChargeAxis = trackTHnSparse->GetAxis(kChargeAxis)->GetXmin(),
           backupMaxChargeAxis = trackTHnSparse->GetAxis(kChargeAxis)->GetXmax();

    if(trackTypeBin > 0) {
        trackTHnSparse->GetAxis(kTrackTypeAxis)->SetRange(trackTypeBin, trackTypeBin);
    }

    if(restrictEMCAL) {
        trackTHnSparse->GetAxis(kEtaAxis)->SetRangeUser(-0.6, 0.6);
        trackTHnSparse->GetAxis(kPhiAxis)->SetRangeUser(1.4, 3.1);
    }

    std::string nametitle;
    TH1 *hAllCharge = trackTHnSparse->Projection(0);
    hAllCharge->SetDirectory(nullptr);
    nametitle = trackTypeString + acceptanceString + "all";
    hAllCharge->SetNameTitle(nametitle.data(), nametitle.data());

    trackTHnSparse->GetAxis(kChargeAxis)->SetRange(1,1);
    TH1 *hNegCharge = trackTHnSparse->Projection(0);
    hNegCharge->SetDirectory(nullptr);
    nametitle = trackTypeString + acceptanceString + "neg";
    hNegCharge->SetNameTitle(nametitle.data(), nametitle.data());

    trackTHnSparse->GetAxis(kChargeAxis)->SetRange(2,2);
    TH1 *hPosCharge = trackTHnSparse->Projection(0);
    hPosCharge->SetDirectory(nullptr);
    nametitle = trackTypeString + acceptanceString + "pos";
    hPosCharge->SetNameTitle(nametitle.data(), nametitle.data());

    // reset ranges
    trackTHnSparse->GetAxis(kEtaAxis)->SetRangeUser(backupMinEtaAxis, backupMaxEtaAxis);
    trackTHnSparse->GetAxis(kPhiAxis)->SetRangeUser(backupMinPhiAxis, backupMaxPhiAxis);
    trackTHnSparse->GetAxis(kTrackTypeAxis)->SetRangeUser(backupMinTrackAxis, backupMaxTrackAxis);
    trackTHnSparse->GetAxis(kChargeAxis)->SetRangeUser(backupMinChargeAxis, backupMaxChargeAxis);

    return {hAllCharge, hNegCharge, hPosCharge};
}

std::vector<HistCollection> loadData(TFile &reader, const std::string_view trigger) {
    std::stringstream listnamebuilder;
    listnamebuilder << "AliEmcalTrackingQATask";
    if(trigger != "INT7") {
        listnamebuilder << "_" << trigger;
    }
    listnamebuilder << "_histos";
    auto histlist = reader.Get<TList>(listnamebuilder.str().data());
    auto trackTHnSpare = static_cast<THnSparse *>(histlist->FindObject("fTracks"));
    trackTHnSpare->Sumw2();
    auto allFullAcceptance = extractSpectra(trackTHnSpare, 0, false),
         allEmcalAcceptance = extractSpectra(trackTHnSpare, 0, true),
         globFullAcceptance = extractSpectra(trackTHnSpare, 1, false),
         globEmcalAcceptance = extractSpectra(trackTHnSpare, 1, true),
         compFullAcceptance = extractSpectra(trackTHnSpare, 2, false),
         compEmcalAcceptance = extractSpectra(trackTHnSpare, 2, true);
    return {allFullAcceptance, allEmcalAcceptance, globFullAcceptance, globEmcalAcceptance, compFullAcceptance, compEmcalAcceptance}; 
}

void write(TFile &output, const std::string_view dirname, const std::vector<HistCollection> &histos) {
    output.mkdir(dirname.data());
    output.cd(dirname.data());
    for(const auto &coll : histos) {
        coll.hAllCharge->Write();
        coll.hNegCharge->Write();
        coll.hPosCharge->Write();
    }
}

void extractTrackSpectraCharges(const std::string_view inputfile = "AnalysisResults.root"){
    std::map<std::string, std::vector<HistCollection>> histhandler;
    {
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        reader->ls();
        std::array<std::string, 3> triggers = {{"INT7", "EJ2", "EJ1"}};
        for(auto &trg : triggers) {
            histhandler[trg] = loadData(*reader, trg);
        }
    }
    {
        std::unique_ptr<TFile> writer(TFile::Open("trackptspectra.root", "RECREATE"));
        for(const auto &[trg, histos] : histhandler) {
            write(*writer, trg, histos);
        }
    }
}