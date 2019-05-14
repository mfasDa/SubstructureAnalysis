#include "../../meta/root.C"
#include "../../struct/ResponseReader.cxx"

void extractProjectionResponseDet(const std::string_view filename) {
    ResponseReader reader(filename);
    std::vector<TH1 *> responseMatrixProjectionsDet,
                       responseMatrixProjectionsPart;
    for(auto r : reader.getJetRadii()) {
        auto hresponse = reader.GetResponseMatrixTruncated(r);
        auto hdet = hresponse->ProjectionX(Form("hdet_R%02d", int(r*10))),
             hpart = hresponse->ProjectionY(Form("hpart_R%02d", int(r*10)));
        hdet->SetDirectory(nullptr);
        hpart->SetDirectory(nullptr);
        hdet->Scale(1., "width");
        hpart->Scale(1., "width");
        responseMatrixProjectionsDet.push_back(hdet);
        responseMatrixProjectionsPart.push_back(hpart);
    }

    std::unique_ptr<TFile> writer(TFile::Open("ResponseMatrixProjections.root", "RECREATE"));
    writer->cd();
    for(auto hist : responseMatrixProjectionsDet) hist->Write();
    for(auto hist : responseMatrixProjectionsPart) hist->Write();
}