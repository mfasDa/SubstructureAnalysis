#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/aliroot.C"

std::vector<std::string> getListOfPeriods(std::string_view basedir, int year) {
    std::vector<std::string> result;
    std::string yeartag = Form("%d", (year % 2000));
    auto searchresult = gSystem->GetFromPipe(Form("ls -1 %s | grep %s | grep -v bad", basedir.data(), yeartag.data()));
    std::unique_ptr<TObjArray> tokens(searchresult.Tokenize("\n"));
    for(auto tok : TRangeDynCast<TObjString>(tokens.get())) {
        result.push_back(tok->String().Data());
    }
    std::sort(result.begin(), result.end(), std::less<std::string>());
    return result;
}

std::vector<int> getRunsYear(int year) {
    std::string repo = gSystem->Getenv("SUBSTRUCTURE_ROOT");
    std::string runlistdir = Form("%s/runlists_EMCAL", repo.data());
    std::vector<int> result;
    for(auto period : getListOfPeriods(runlistdir, year)) {
        std::string runlistfull = Form("%s/%s", runlistdir.data(), period.data());
        std::string line, entry;
        std::ifstream reader(runlistfull);
        while(std::getline(reader, line)) {
            std::stringstream parser(line);
            while(std::getline(parser, entry, ',')) {
                if(!entry.length()) continue;
                try {
                    result.push_back(std::stoi(entry));
                }
                catch(...) {
                    std::cout << "Failed adding " << entry << std::endl;
                }
            }
        }
    }

    std::sort(result.begin(), result.end(), std::less<int>());
    return result;
}

std::map<int, int> getSTURegions(AliCDBManager *cdb, int runnumber) {
    std::map<int, int> result;
    cdb->SetRun(runnumber);
    auto trgen = cdb->Get("EMCAL/Calib/Trigger");
    auto trgdcs = static_cast<AliEMCALTriggerDCSConfig *>(trgen->GetObject());
    auto emcalSTU = trgdcs->GetSTUDCSConfig(false),
         dcalSTU = trgdcs->GetSTUDCSConfig(true); 
    std::bitset<32> emcalregion(0), dcalregion(0);
    if(emcalSTU) emcalregion = std::bitset<32>(emcalSTU->GetRegion());
    if(dcalSTU) dcalregion = std::bitset<32>(dcalSTU->GetRegion());
    for(auto iTRU : ROOT::TSeqI(0, 32)) {
        result[iTRU] = emcalregion.test(iTRU) ? 1 : 0;
    }
    for(auto iTRU : ROOT::TSeqI(0, 20)) {
        result[iTRU+32] = dcalregion.test(iTRU) ? 1 : 0;
    }
    return result;
}

void makeTrendSTURegion(int year){
    AliCDBManager *cdb = AliCDBManager::Instance();
    cdb->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", year));
    std::array<TGraph *, 52> trugraphs;
    for(auto itru : ROOT::TSeqI(0, 52)) {
        trugraphs[itru] = new TGraph;
        trugraphs[itru]->SetName(Form("TRU%d", itru));
    }
    for(auto run : getRunsYear(year)) {
        auto trustatus = getSTURegions(cdb, run);
        for(auto itru : ROOT::TSeqI(0, 52)){
            trugraphs[itru]->SetPoint(trugraphs[itru]->GetN(), run, trustatus[itru]);
        }
    }

    std::unique_ptr<TFile> writer(TFile::Open(Form("TRUStatus%d.root", year), "RECREATE"));
    for(auto trugraph : trugraphs) {
        trugraph->Write();
    }
}