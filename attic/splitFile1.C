#include "meta/stl.C"
#include "meta/root.C"
#include "helpers/filesystem.C"

void splitFile1(const std::string_view inputfile = "AnalysisResults.root") {
    std::string dn = dirname(inputfile);
    if(!dn.length()) dn = gSystem->GetWorkingDirectory();
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    std::unique_ptr<TFile> writerNormal(TFile::Open(Form("%s/AnalysisResults_split.root",dn.data()), "RECREATE")),
                           writerTracking(TFile::Open(Form("%s/TrackingQA.root", dn.data()), "RECREATE"));
    TFile *currentfile(nullptr);
    for(auto c : TRangeDynCast<TKey>(reader->GetListOfKeys())){
        if(TString(c->GetName()).Contains("AliEmcalTrackingQATask_histos")) {
            currentfile = writerTracking.get();
        } else {
            currentfile = writerNormal.get();
        }
        if(dynamic_cast<TDirectoryFile *>(c->ReadObj())) {
            currentfile->mkdir(c->GetName());
            reader->cd(c->GetName());
            for(auto cont : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
                currentfile->cd(c->GetName());
                cont->ReadObj()->Write(cont->GetName(), TObject::kSingleKey);
            }
        }
        currentfile->cd();
        auto o = c->ReadObj();
        o->Write(o->GetName(), TObject::kSingleKey);
    }
}