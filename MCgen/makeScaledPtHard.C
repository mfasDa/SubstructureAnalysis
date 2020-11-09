#include "../meta/root.C"
#include "../meta/stl.C"

std::vector<std::string> scalehists = {
    "fHistTrialsAfterSel",
    "fHistEventsAfterSel",
    "fHistXsectionAfterSel",
    "fHistTrials",
    "fHistEvents",
    "fHistXsection"

};

TList *scaleList(TList *list, int pthardbin)
{
    auto xsechist = static_cast<TH1 *>(list->FindObject("fHistXsection")),
         ntrialshist = static_cast<TH1 *>(list->FindObject("fHistTrials"));
    double weight = xsechist->GetBinContent(1) / ntrialshist->GetBinContent(1);

    TList *outlist = new TList;
    outlist->SetName(list->GetName());
    std::vector<double> binningscale;
    for(double binlimit = -0.5; binlimit <= 20.5; binlimit += 1) binningscale.push_back(binlimit);

    for (auto hist : TRangeDynCast<TH1>(static_cast<TList *>(list)))
    {
        TH1 *outhist (nullptr);
        if (std::find(scalehists.begin(), scalehists.end(), hist->GetName()) != scalehists.end()) {
            outhist = new TH1D(hist->GetName(), hist->GetTitle(), binningscale.size()-1, binningscale.data());
            auto val = hist->GetBinContent(1),
                 err = hist->GetBinError(1);
            auto outbin = outhist->GetXaxis()->FindBin(pthardbin);
            outhist->SetBinContent(outbin, val);
            outhist->SetBinError(outbin, err);
        } else {
            hist->Scale(weight);
            outhist = hist;
        }
        outlist->Add(outhist);
    }
    return outlist;
}

std::vector<TList *> readFile(const char *name, int pthardbin)
{
    TFile *reader = TFile::Open(name, "READ");
    std::vector<TList *> data;
    for (auto key : TRangeDynCast<TKey>(reader->GetListOfKeys()))
    {
        auto histlist = key->ReadObject<TList>();
        data.push_back(scaleList(histlist, pthardbin));
    }
    return data;
}

void makeScaledPtHard(int pthardbin, const char *filename = "AnalysisResults.root") {
    std::unique_ptr<TFile> outwriter(TFile::Open("AnalysisResults_scaled.root", "RECREATE"));
    auto lists = readFile(filename, pthardbin);
    outwriter->cd();
    for(auto data : lists) data->Write(data->GetName(), TObject::kSingleKey);
}