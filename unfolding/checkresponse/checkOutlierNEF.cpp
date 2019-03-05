struct OutlierSelection {
    int pthardbin;
    double detPtMax;
    double partPtMin;
    
    bool operator==(const OutlierSelection &other) const { return pthardbin == other.pthardbin; }
    bool operator<(const OutlierSelection &other) const { return pthardbin < other.pthardbin; }
};

std::vector<OutlierSelection> cutsR02 = {
    {1, 10., 20.}, {2, 20., 25.}, {3, 20., 30.}, {4, 30., 40.}, {5, 40., 50.}, {6, 50., 70.},
    {7, 60., 80.}, {8, 80., 100.}, {9, 100., 120.}, {10, 120., 140.}, {11, 140., 180.}, {12, 150., 200.},
    {13, 300., 250.}, {14, 300., 270.}, {15, 300., 300.}, {16, 300., 350.}, {17, 300., 380.}, {18, 300., 420.}, {19, 300., 450.}, {20, 300., 480.}
};

TH1 *GetNEFOutlier(int pthardbin) {
    std::unique_ptr<TFile> reader(TFile::Open(Form("%02d/AnalysisResults.root", pthardbin), "READ"));
    reader->cd("EnergyScaleResults_FullJet_R02_INT7");
    auto histlist = static_cast<TList *>(gDirectory->Get("EnergyScaleHists_FullJet_R02_INT7"));
    auto hs = static_cast<THnSparse *>(histlist->FindObject("hPtCorr"));
    auto conf = std::find_if(cutsR02.begin(), cutsR02.end(), [pthardbin](const OutlierSelection &test) { return test.pthardbin == pthardbin; } );
    hs->GetAxis(0)->SetRangeUser(conf->partPtMin, 500.);
    hs->GetAxis(1)->SetRangeUser(0., conf->detPtMax);
    hs->GetAxis(3)->SetRangeUser(0., 0.099);
    auto nef = hs->Projection(2);
    nef->SetDirectory(nullptr);
    nef->SetName(Form("outiierNEF_%d", pthardbin));
    return nef;
}

void checkOutlierNEF() {
    std::vector<ROOT6tools::TSavableCanvas *> cache;
    ROOT6tools::TSavableCanvas *currentcanvas = nullptr;
    int currentpad = 1, currentCanvasID = 1;
    for(auto pthard : ROOT::TSeqI(1, 21)){
        if(!currentcanvas || currentpad == 11) {
            currentcanvas = new ROOT6tools::TSavableCanvas(Form("OutlierNEFR02_%d", currentCanvasID), Form("NEF distribution for outlier jets %d", currentCanvasID), 1200, 800);
            currentcanvas->Divide(5,2);
            cache.push_back(currentcanvas);
            currentCanvasID++;
            currentpad = 1;
        }

        currentcanvas->cd(currentpad++);
        auto nefdist = GetNEFOutlier(pthard);
        nefdist->SetStats(false);
        nefdist->SetXTitle("NEF");
        nefdist->SetYTitle("Yield");
        nefdist->SetTitle("");
        nefdist->Draw();

        auto conf = std::find_if(cutsR02.begin(), cutsR02.end(), [pthard](const OutlierSelection &test) { return test.pthardbin == pthard; } );
        auto label = new ROOT6tools::TNDCLabel(0.15, 0.7, 0.89, 0.89, Form("pt-hard bin %d", pthard));
        label->AddText(Form("p_{t, part} > %.1f GeV/c, p_{t, det} < %.1f GeV/c", conf->partPtMin, conf->detPtMax));
        label->SetTextAlign(12);
        label->Draw();
    }

    for(auto c : cache) {
        c->cd();
        c->Update();
        c->SaveCanvas(c->GetName());
    }
}