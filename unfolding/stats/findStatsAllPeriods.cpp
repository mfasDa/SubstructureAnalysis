#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/roounfold.C"
#include "../../helpers/filesystem.C"
#include "../../helpers/string.C"

std::vector<std::string> getListOfPeriods(const std::string_view inputdir){
    auto dirs = tokenize(gSystem->GetFromPipe(Form("ls -1 %s | grep LHC", inputdir.data())).Data(), '\n');
    std::sort(dirs.begin(), dirs.end());
    return dirs;
}

std::array<int, 3> getnevents(const std::string_view filename, bool triggers){
    std::array<int, 3> result = {{0, 0, 0}};
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    reader->cd("JetSubstructure_FullJets_R02_INT7");
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto evcount = static_cast<TH1 *>(histlist->FindObject("hEventCounter"));
    result[0] = evcount->GetBinContent(1);
    if(triggers){
        reader->cd("JetSubstructure_FullJets_R02_EJ1");
        histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        evcount = static_cast<TH1 *>(histlist->FindObject("hEventCounter"));
        result[1] = evcount->GetBinContent(1);

        reader->cd("JetSubstructure_FullJets_R02_EJ2");
        histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        evcount = static_cast<TH1 *>(histlist->FindObject("hEventCounter"));
        result[2] = evcount->GetBinContent(1);
    }
    return result;
}

void findStatsAllPeriods(){
    std::array<int, 3> sum = {{0,0,0}};
    for(auto &p : getListOfPeriods(gSystem->GetWorkingDirectory())){
        auto ev = getnevents(Form("%s/AnalysisResults.root", p.data()), contains(p, "17"));
        //std::cout << p << ": INT7(" << ev[0] << "), EJ1(" << ev[1] << "), EJ2(" << ev[2] << ")" << std::endl;
        std::cout << p << " & " << std::fixed << std::setprecision(1) << double(ev[0]) / 1e6  << " & " << double(ev[1]) / 1e6 << " & " << double(ev[2])/1e6 << " \\\\" << std::endl;
        for(auto i : ROOT::TSeqI(0,3)) sum[i] += ev[i];
    }
    std::cout << "Sum" << " & " << std::fixed << std::setprecision(1) << double(sum[0]) / 1e6  << " & " << double(sum[1]) / 1e6 << " & " << double(sum[2])/1e6 << " \\\\" << std::endl;
}