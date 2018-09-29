#include "../../meta/root.C"
#include "../../meta/root6tools.C"

const std::vector<std::string> triggers = {"INT7", "EJ1"};

std::map<std::string, TH1 *> read(const std::string_view inputfile){
    std::map<std::string, TH1 *> spectre;
    std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
    reader->cd("rawlevel");
    for(const auto &t :triggers) {
        auto spec = static_cast<TH1 *>(gDirectory->Get(Form("%s", t.data())));
    }
    return spectre;
}

void makeCheckNewNonLinCorr(){
    
}