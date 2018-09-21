#include "meta/root.C"

void convertRDF(const std::string_view filename, const std::string_view treename, int pthardbin){
    ROOT::EnableImplicitMT(10);
    std::unique_ptr<TFile> filereader(TFile::Open(filename.data(), "READ"));

    std::string listname = std::string(treename);
    listname.erase(listname.find("Tree"), 4);
    filereader->cd(listname.data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto xsection = static_cast<TH1 *>(histlist->FindObject("fHistXsection"))->GetBinContent(pthardbin+1);
    auto nTrials = static_cast<TH1 *>(histlist->FindObject("fHistTrials"))->GetBinContent(pthardbin+1);
    auto nEvents = static_cast<TH1 *>(histlist->FindObject("fHistEvents"))->GetBinContent(pthardbin+1);
    auto weightPythia  = xsection / nTrials;

    std::cout << "Pt-hard bin " << pthardbin << "found weight " << weightPythia << std::endl;
    
    ROOT::RDataFrame df(treename.data(), filename.data());
    df.Define("pthardbin", [&pthardbin](){ return pthardbin; }, {})
      .Define("weightPythiaFromPtHard", [&weightPythia](){return weightPythia;}, {})
      .Snapshot("jetSubstructure", Form("%s_pt%02d.root", treename.data(), pthardbin));
}