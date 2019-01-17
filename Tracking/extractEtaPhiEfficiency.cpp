void extractEtaPhiEfficiency(const std::string_view tracktype, const std::string_view trigger, const std::string_view inputfile, double ptmin, double ptmax) {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    reader->cd(Form("ChargedParticleQA_%s", tracktype.data()));
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto hsparseTrue = static_cast<THnSparse *>(histlist->FindObject("hPtEtaPhiAllTrue"));
    auto hsparse = static_cast<THnSparse *>(histlist->FindObject(Form("hPtEtaPhiAll%s", trigger.data())));
    int binptmin = hsparse->GetAxis(0)->FindBin(ptmin), binptmax = hsparse->GetAxis(0)->FindBin(ptmax);
    double valptmin = hsparse->GetAxis(0)->GetBinLowEdge(binptmin), valptmax = hsparse->GetAxis(0)->GetBinUpEdge(binptmax);
    hsparse->GetAxis(0)->SetRange(binptmin, binptmax);
    hsparseTrue->GetAxis(0)->SetRange(binptmin, binptmax);
    auto etaphi = hsparse->Projection(1,2);
    auto etaphiTrue = hsparseTrue->Projection(1,2);
    etaphi->Divide(etaphiTrue);
    etaphi->SetDirectory(nullptr);
    etaphi->SetStats(false);
    etaphi->SetXTitle("#phi");
    etaphi->SetYTitle("#eta");
    etaphi->SetTitle(Form("%s tracks, %.1f GeV/c < p_{t} < %.1f GeV/c", tracktype.data(), valptmin, valptmax));
    etaphi->Draw("colz");
}