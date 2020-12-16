#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/aliphysics.C"

std::vector<std::string> scalehists = {
    "fHistTrialsAfterSel",
    "fHistEventsAfterSel",
    "fHistXsectionAfterSel",
    "fHistTrials",
    "fHistEvents",
    "fHistXsection"

};

void scaleList(AliEmcalList *list)
{
    auto xsechist = static_cast<TH1 *>(list->FindObject("fHistXsection")),
         ntrialshist = static_cast<TH1 *>(list->FindObject("fHistTrials"));

    double weight = xsechist->GetBinContent(1) / ntrialshist->GetBinContent(1);
    for (auto hist : TRangeDynCast<TH1>(static_cast<TList *>(list)))
    {
        if (std::find(scalehists.begin(), scalehists.end(), hist->GetName()) != scalehists.end())
            continue;
        hist->Scale(weight);
    }
}

std::vector<AliEmcalList *> readFile(const char *name)
{
    std::cout << "Reading " << name << std::endl;
    TFile *reader = TFile::Open(name, "READ");
    std::vector<AliEmcalList *> data;
    for (auto key : TRangeDynCast<TKey>(reader->GetListOfKeys()))
    {
        auto histlist = key->ReadObject<AliEmcalList>();
        scaleList(histlist);
        data.push_back(histlist);
    }
    return data;
}

AliEmcalList *createMerged(std::map<int, AliEmcalList *> &pthardbins)
{
    AliEmcalList *result = new AliEmcalList;
    result->SetName(pthardbins[1]->GetName());
    std::vector<double> binning = {-0.5};
    for (int ib = 0; ib < 21; ib++)
    {
        binning.push_back(double(ib) + 0.5);
    }
    std::vector<TH1 *> mergedscalehists;
    for (auto scalehist : scalehists)
    {
        auto inputhist = static_cast<TH1 *>(pthardbins[1]->FindObject(scalehist.data()));
        auto mergedscalehist = new TH1D(inputhist->GetName(), inputhist->GetTitle(), binning.size() - 1, binning.data());
        mergedscalehists.push_back(mergedscalehist);
    }
    std::vector<TH1 *> histos;
    for (auto [pthardbin, inputhistos] : pthardbins)
    {
        for (auto inputhist : TRangeDynCast<TH1>(inputhistos))
        {
            if (std::find(scalehists.begin(), scalehists.end(), inputhist->GetName()) != scalehists.end())
            {
                // histo is scale hist
                auto mergedscalehist = *std::find_if(mergedscalehists.begin(), mergedscalehists.end(), [inputhist](const TH1 *hist) { return std::string_view(inputhist->GetName()) == std::string_view(hist->GetName()); });
                auto mergedbin = mergedscalehist->GetXaxis()->FindBin(pthardbin);
                mergedscalehist->SetBinContent(mergedbin, inputhist->GetBinContent(1));
                mergedscalehist->SetBinError(mergedbin, inputhist->GetBinError(1));
            }
            else
            {
                // histo is regular hist
                auto histname = inputhist->GetName();
                auto found = std::find_if(histos.begin(), histos.end(), [histname](const TH1 *histo) { return std::string_view(histname) == std::string_view(histo->GetName()); });
                if (found == histos.end())
                {
                    auto merged = static_cast<TH1 *>(inputhist->Clone());
                    histos.push_back(merged);
                }
                else
                {
                    auto hist = *found;
                    hist->Add(inputhist);
                }
            }
        }
    }
    for (auto scalehist : mergedscalehists)
        result->Add(scalehist);
    for (auto hist : histos)
        result->Add(hist);
    return result;
}

void mergePtHardBins(const char *filename = "AnalysisResults.root", int maxbin = 20)
{
    std::map<int, std::vector<AliEmcalList *>> pthardbins;
    std::vector<std::string> histnames;
    for (auto ipth : ROOT::TSeqI(1, maxbin+1))
    {
        auto histlists = readFile(Form("%02d/%s", ipth, filename));
        if (ipth == 1)
        {
            for (auto hist : histlists)
            {
                histnames.push_back(hist->GetName());
            }
        }
        pthardbins[ipth] = histlists;
    }

    std::unique_ptr<TFile> writer(TFile::Open(filename, "RECREATE"));
    for (auto hl : histnames)
    {
        std::map<int, AliEmcalList *> pth;
        for (auto [phb, histlists] : pthardbins)
        {
            auto *histlist = *std::find_if(histlists.begin(), histlists.end(), [hl](const AliEmcalList *list) { return std::string(list->GetName()) == hl; });
            pth[phb] = histlist;
        }
        auto merged = createMerged(pth);
        merged->Write(merged->GetName(), TObject::kSingleKey);
    }
}
