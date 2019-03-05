#include "../../helpers/filesystem.C"
void extractOutlierTree(const std::string_view inputfile = "AnalysisResults.root") {
    std::string basedir = dirname(inputfile);
    std::stringstream outfile;
    if(basedir.length()) outfile << basedir << "outliers.root";
    else outfile << "outliers.root";
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(),"READ"));
    reader->cd("OutlierResponse");
    auto histlist = static_cast<TList*>(gDirectory->Get("OutlierResponseHists"));
    auto outliertree = static_cast<TTree *>(histlist->FindObject("fOutlierData"));

    auto xsechist =   static_cast<TH1 *>(histlist->FindObject("fHistXsection")),
         trialshist = static_cast<TH1 *>(histlist->FindObject("fHistTrials"));

    int pthardbin(0);
    for(auto b : ROOT::TSeqI(0, trialshist->GetXaxis()->GetNbins())) {
        if(trialshist->GetBinContent(b+1)) {
            pthardbin = b;
            break;
        }
    }
    auto weight = xsechist->GetBinContent(pthardbin+1)/trialshist->GetBinContent(pthardbin+1);

    // Scale tree
    ROOT::RDataFrame resultframe(*outliertree);
    resultframe.Define("pthardbin", [pthardbin](){return pthardbin;})
               .Define("weight", [weight](){return weight;})
               .Snapshot("OutlierTree", outfile.str().data());
    
}