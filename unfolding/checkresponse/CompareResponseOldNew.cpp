#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/unfolding.C"

TH2 * ReadResponseOld(const std::string_view filename){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("detectorresponse");
    auto responsematrix = static_cast<TH2 *>(gDirectory->Get("responseMatrix"));
    responsematrix->SetDirectory(nullptr);
    return responsematrix;
}

std::map<double, TH2 *> ReadResponseNew(const std::string_view filename) {
    std::map<double, TH2 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(auto r : ROOT::TSeqI(2, 6)){
        reader->cd(Form("R%02d", r));
        gDirectory->cd("response");
        auto hresponse = static_cast<TH2 *>(gDirectory->Get(Form("Rawresponse_R%02d_rebinned", r)));
        hresponse->SetDirectory(nullptr);
        result[double(r)/10.] = hresponse;
    }
    return result;
}

void CompareResponseOldNew(){
    std::map<double, TH2 *> responseOld, 
                            responseNew = ReadResponseNew("correctedSVD.root");

    TH2 *htemplate = responseNew[0.2];
    for(auto r : ROOT::TSeqI(2, 6)) {
        std::unique_ptr<TH2> htmp(ReadResponseOld(Form("corrected1DSVD_R%02d.root", r)));
        auto truncated = TruncateResponse(htmp.get(), htemplate, Form("Oldresponse_truncated_R%02d", r), htmp->GetTitle());
        truncated->SetDirectory(nullptr);
        responseOld[double(r)/10.] = truncated;
    }

    auto plot = new ROOT6tools::TSavableCanvas("ComparisonResponse", "Comparison response matrices", 1200, 1000);
    plot->Divide(2,2);

    int ipad(1);
    for(auto [k, v] : responseOld){
        auto nres = responseNew.find(k)->second;
        nres->Divide(v);

        plot->cd(ipad++);
        nres->SetStats(false);
        nres->Draw("colz");
        nres->GetZaxis()->SetRangeUser(0, 3);
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}