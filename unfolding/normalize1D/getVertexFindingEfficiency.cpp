#include "../../meta/root.C"
void getVertexFindingEfficiency(const std::string_view filename = "AnalysisResults_split.root"){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("JetSubstructure_FullJets_R02_INT7");
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto histaccepted = static_cast<TH1 *>(histlist->FindObject("fHistEventCount")),
         histrejection = static_cast<TH1 *>(histlist->FindObject("fHistEventRejection"));
    double accepted = histaccepted->GetBinContent(histaccepted->GetXaxis()->FindBin("Accepted")),
           rejectedVz = histrejection->GetBinContent(histrejection->GetXaxis()->FindBin("Vz")),
           rejectedVertexCuts = histrejection->GetBinContent(histrejection->GetXaxis()->FindBin("VtxSel2013pA"));
    double accIncVz = accepted + rejectedVz, allIncVz = accepted + rejectedVz + rejectedVertexCuts;
    double efficiency = accIncVz / allIncVz;
    std::cout << "Vertex finding efficiency: " << efficiency << std::endl;
}