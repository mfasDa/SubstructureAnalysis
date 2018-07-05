#ifndef __CLING__
#include <memory>
#include <TFile.h>
#include <TKey.h>
#include <TTree.h>
#endif
void WriteTree(TTree *t) {
    std::stringstream filename;
    filename << t->GetName() << ".root";
    std::unique_ptr<TFile> writer(TFile::Open(filename.str().c_str(), "RECREATE"));
    writer->cd();
    t->CloneTree()->Write();
}

void splitFile(Bool_t writeTrees = true) {
    std::unique_ptr<TFile> reader(TFile::Open("AnalysisResults.root", "READ"));
    std::unique_ptr<TFile> writer(TFile::Open("AnalysisResults_split.root", "RECREATE"));
    for(auto c : TRangeDynCast<TKey>(reader->GetListOfKeys())){
        if(dynamic_cast<TTree *>(c->ReadObj())) continue;
        if(dynamic_cast<TDirectoryFile *>(c->ReadObj())) {
            writer->mkdir(c->GetName());
            reader->cd(c->GetName());
            for(auto cont : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())){
                writer->cd(c->GetName());
                cont->ReadObj()->Write(cont->GetName(), TObject::kSingleKey);
            }
        }
        writer->cd();
        auto o = c->ReadObj();
        o->Write(o->GetName(), TObject::kSingleKey);
    }
    writer->Close();
    if(writeTrees){
        for(auto c : TRangeDynCast<TKey>(reader->GetListOfKeys())){
            if(auto t = dynamic_cast<TTree *>(c->ReadObj())){
                WriteTree(t);
            } 
        }
    }
}