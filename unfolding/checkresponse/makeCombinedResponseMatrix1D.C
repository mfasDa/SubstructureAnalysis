#include "../../meta/stl.C"
#include "../../meta/aliphysics.C"
#include "../../meta/root.C"

struct DirContent {
    TString dirname;
    AliEmcalList *histlist;
};

int parseR(const TString &keyname) {
    int R = 0;
    std::unique_ptr<TObjArray> tokens(keyname.Tokenize("_"));
    for(auto tok : TRangeDynCast<TObjString>(tokens.get())) {
        auto tokstr = tok->String();
        if(tokstr.Contains("R")) {
            auto rstring = tokstr.ReplaceAll("R", "");
            R = rstring.Atoi();
        }
    }
    return R;
}

std::vector<TString> scalehists = {
    "fHistTrialsAfterSel",
    "fHistEventsAfterSel",
    "fHistXsectionAfterSel",
    "fHistTrials",
    "fHistEvents",
    "fHistXsection"
};

void scaleMinBias(AliEmcalList *list) {
    auto xsechist = static_cast<TH1 *>(list->FindObject("fHistXsection")),
         trialshist = static_cast<TH1 *>(list->FindObject("fHistTrials"));
    auto weight = xsechist->GetBinContent(1) / trialshist->GetBinContent(1);
    for(auto obj : *list) {
        if(std::find(scalehists.begin(), scalehists.end(), obj->GetName()) != scalehists.end()) {
            // don't scale scale hists
            continue;
        }
        if(obj->InheritsFrom(TH1::Class())) {
            std::cout << "scaling TH1 based object" << std::endl;
            auto hist = static_cast<TH1 *>(obj);
            hist->Scale(weight);
        } else if (obj->InheritsFrom(THnSparse::Class())) {
            std::cout << "scaling THnSparse based object" << std::endl;
            auto hist = static_cast<THnSparse *>(obj);
            hist->Scale(weight);
        }
    }
}

std::map<int, DirContent> readMinBias(const char *filename) {
    std::map<int, DirContent> result;
    TFile *reader = TFile::Open(filename, "READ");
    for(auto entry : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
        TString keyname = entry->GetName();
        if(!keyname.Contains("EnergyScaleResults") && !keyname.Contains("SoftDropResponse")) continue;
        auto R = parseR(keyname);
        reader->cd(keyname.Data());
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<AliEmcalList>();
        scaleMinBias(histlist);
        result[R] = {keyname, histlist};
    }
    return result;
}

std::map<int, DirContent> readJetJet(const char *filename) {
    std::map<int, DirContent> result;
    TFile *reader = TFile::Open(filename, "READ");
    for(auto entry : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
        TString keyname = entry->GetName();
        if(!keyname.Contains("EnergyScaleResults") && !keyname.Contains("SoftDropResponse")) continue;
        auto R = parseR(keyname);
        reader->cd(keyname.Data());
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<AliEmcalList>();
        result[R] = {keyname, histlist};
    }
    return result;
}

AliEmcalList *combineSamples(const AliEmcalList *minbiassample, const AliEmcalList *jjsample) {
    AliEmcalList *result = new AliEmcalList;
    result->SetName(jjsample->GetName());
    for(auto hist : *jjsample) {
        //std::cout << "Combining " << hist->GetName() << std::endl;
        TObject *outputobject = nullptr;
        auto *mbhist = minbiassample->FindObject(hist->GetName());   
        if(std::find(scalehists.begin(), scalehists.end(), hist->GetName()) != scalehists.end()) {
            // Histogram is a scale hist, put min. bias in bin 0 and keep other bins
            TH1 *inhist = static_cast<TH1 *>(hist),
                *mbinhist = static_cast<TH1 *>(mbhist);
            auto outhist = new TH1D(hist->GetName(), hist->GetTitle(), inhist->GetXaxis()->GetNbins(), inhist->GetXaxis()->GetXbins()->GetArray());
            outhist->SetDirectory(nullptr);
            for(auto ib : ROOT::TSeqI(0, inhist->GetXaxis()->GetNbins())){
                outhist->SetBinContent(ib+1, inhist->GetBinContent(ib+1));
                outhist->SetBinError(ib+1, inhist->GetBinError(ib+1));
            }
            outhist->SetBinContent(1, mbinhist->GetBinContent(1));
            outhist->SetBinError(1, mbinhist->GetBinError(1));
            outputobject = outhist;
        } else if (TString(hist->GetName()).Contains("fHistPtHardBinCorr") || TString(hist->GetName()).Contains("fHistPtHardCorr")){
            auto outhist = static_cast<TH1 *>(hist->Clone());
            outhist->SetDirectory(nullptr);
            outputobject = outhist;
        } else {
            if(hist->InheritsFrom(TH1::Class())) {
                auto outhist = static_cast<TH1 *>(hist->Clone()),
                     mbinhist = static_cast<TH1 *>(mbhist);
                outhist->SetDirectory(nullptr);
                outhist->Add(mbinhist);
                outputobject = outhist;
            } else if(hist->InheritsFrom(THnSparse::Class())) {
                auto outhist = static_cast<THnSparse *>(hist->Clone()),
                     mbinhist = static_cast<THnSparse *>(mbhist);
                outhist->Add(mbinhist);
                outputobject = outhist;
            }
        }
        result->Add(outputobject);
    }
    return result;
}

void makeCombinedResponseMatrix1D(const char *filemb, const char *filejj, const char *outfile = "mergedresponse.root"){
    auto mbdata = readMinBias(filemb),
         jetjetdata = readJetJet(filejj);

    std::unique_ptr<TFile> resultfile(TFile::Open(outfile, "RECREATE"));
    for(auto &[R, workdir] : jetjetdata) {
        auto mbdir = mbdata[R];
        auto mergedlist = combineSamples(mbdir.histlist, workdir.histlist);
        resultfile->mkdir(workdir.dirname.Data());
        resultfile->cd(workdir.dirname.Data());
        mergedlist->Write(mergedlist->GetName(), TObject::kSingleKey);   
    }
}