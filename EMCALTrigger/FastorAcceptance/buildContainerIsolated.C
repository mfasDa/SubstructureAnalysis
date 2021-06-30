#include "../../meta/stl.C"
#include "../../meta/root.C"

#ifndef __CLING__
#include "AliEmcalFastorMaskContainer.h"
#include "AliOADBContainer.h"
#endif

std::set<int> getRuns(const char *basedir, int year) {
    std::string workdir = Form("%s/%d", basedir, year);
    std::unique_ptr<TObjArray> files(gSystem->GetFromPipe(Form("ls -1 %s | grep fastORsGrouped", workdir.data())).Tokenize("\n"));
    std::set<int> runnumbers;
    for(auto fname : TRangeDynCast<TObjString>(files.get())) {
        std::string_view filestring(fname->String().Data());
        auto runstring = filestring.substr(filestring.find("_") + 1, 6);
        runnumbers.insert(std::stoi(runstring.data()));
    }
    return runnumbers;
}

std::set<int> readMask(const char *filename) {
    std::ifstream reader(filename);
    std::string tmp;
    std::set<int> channels;
    while(std::getline(reader, tmp)) channels.insert(std::stoi(tmp));
    return channels;
}

void buildContainerIsolated(const char *inputdir = ""){
    std::vector<int> years = {2016, 2017, 2018};
    std::string workdir = inputdir;
    if(!workdir.length()) {
        std::cout << "Using current directory as working directory" << std::endl;
        workdir = gSystem->pwd();
    }

    std::string oadbname = "AliEmcalMaskedFastors";
    AliOADBContainer oadbIsolated(oadbname.data()),
                     oadbGrouped(oadbname.data());
    for(auto year : years) {
        for(auto run : getRuns(workdir.data(), year)) {
            auto containerIsolated = new PWG::EMCAL::AliEmcalFastorMaskContainer(Form("%d_%d", year, run)),
                 containerGrouped = new PWG::EMCAL::AliEmcalFastorMaskContainer(Form("%d_%d", year, run));
            containerIsolated->SetRunType(PWG::EMCAL::AliEmcalFastorMaskContainer::RunType_t::kRun2);
            containerGrouped->SetRunType(PWG::EMCAL::AliEmcalFastorMaskContainer::RunType_t::kRun2);
            for(auto channelIsolated : readMask(Form("%s/%d/fastORsIsolated_%d.txt", workdir.data(), year, run))) {
                containerIsolated->AddDeadFastor(channelIsolated);
            }
            for(auto channelGrouped : readMask(Form("%s/%d/fastORsGrouped_%d.txt", workdir.data(), year, run))) {
                containerGrouped->AddDeadFastor(channelGrouped);
            }
            oadbIsolated.AppendObject(containerIsolated, run, run);
            oadbGrouped.AppendObject(containerGrouped, run, run);
        }
    }
    oadbIsolated.WriteToFile(Form("%s/MaskedFastorsIsolated.root", workdir.data()));
    oadbGrouped.WriteToFile(Form("%s/MaskedFastorsGrouped.root", workdir.data()));
}