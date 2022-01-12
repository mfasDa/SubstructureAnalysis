#ifndef __RUNDB_C__
#define __RUNDB_C__

#ifndef __CLING__
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <TObjArray.h>
#include <TObjString.h>
#include <TSystem.h>
#endif

#include "string.C"

class RunDB {
    public:
        struct RunInfo {
            std::string period;
            bool good;
        };

        class RunNotFoundException : public std::exception {
            public:
                RunNotFoundException(int runnumber): mRunNumber(runnumber) { 
                    mMessage = Form("No entry found for run %d", mRunNumber);
                }
                ~RunNotFoundException() noexcept override = default;

                const char *what() const noexcept override {
                    return mMessage.data();
                }

                int getRunNumber() const noexcept { return mRunNumber; }
            private:
                std::string mMessage;
                int mRunNumber;
        };

        RunDB(const char *repository = "") : mRunList() { build(repository); }
        ~RunDB() = default;

        RunInfo findRun(int runnumber) {
            auto result = mRunList.find(runnumber);
            if(result == mRunList.end()) { throw RunNotFoundException(runnumber); }
            return result->second;
        }

        std::string getPeriod(int runnumber) {
            auto result = findRun(runnumber);
            return result.period;
        }

        bool isGoodRun(int runnumber) {
            auto result = findRun(runnumber);
            return result.good;
        }

        int getPeriodIndex(const std::string &period) {
            int result = -1;
            auto found = mPeriodIndex.find(period);
            if(found != mPeriodIndex.end()) {
                result = found->second;
            }
            return result;
        }

    private:
        void build(const std::string_view repository = "") {
            mRunList.clear();
            std::string repopath = gSystem->Getenv("SUBSTRUCTURE_ROOT");
            if(repository.length()) repopath = repository.data();
            std::string basedir = Form("%s/runlists_EMCAL", repopath.data());
            int mPeriodID = 0;
            for(auto fl : find_files(basedir.data())) {
                std::string nextfile = Form("%s/%s", basedir.data(), fl.data());
                bool isgood = true;
                if(fl.find("bad") != std::string::npos) isgood = false;
                std::string period = fl;
                period.erase(std::remove(period.begin(), period.end(), ' '), period.end());
                if(period.find("_") != std::string::npos) period = period.substr(0, period.find("_")-1);
                if(period.length() > 6) continue; // skip merged datasets
                if(mPeriodIndex.find(period) == mPeriodIndex.end()) {
                    std::cout << "Adding period " << period << " with index " << mPeriodID << std::endl;
                    mPeriodIndex[period] = mPeriodID;
                    mPeriodID++;
                }
                for(auto r : read_input(nextfile)) {
                    if(mRunList.find(r) == mRunList.end()) {
                        mRunList[r] = {period, isgood};
                    } else {
                        std::cout << "RunDB buider: Skipping double entry for run " << r << std::endl;
                    }
                }
            }
        }

        std::vector<int> read_input(const std::string_view filename) {
            std::cout << "Reading file" << filename << std::endl;
            std::vector<int> result;
            std::ifstream reader(filename.data());
            std::string buffer;
            while(std::getline(reader, buffer)) {
                for(auto &runstr : tokenize(buffer, ',')){
                    runstr.erase(std::remove(runstr.begin(), runstr.end(), ' '), runstr.end());
                    if(!runstr.length()) continue;
                    result.emplace_back(std::stoi(runstr));
                }
            }
            std::sort(result.begin(), result.end(), std::less<int>());
            return result;
        }

        std::vector<std::string> find_files(const std::string_view repository) {
            std::vector<std::string> result;
            std::unique_ptr<TObjArray> files(gSystem->GetFromPipe(Form("ls -1 %s", repository.data())).Tokenize("\n")); 
            for(auto fl : TRangeDynCast<TObjString>(files.get())) {
                if(!fl) continue;
                result.emplace_back(fl->String().Data());
            } 
            std::sort(result.begin(), result.end(), std::less<std::string>());
            return result;
        }

        std::map<int, RunInfo> mRunList;
        std::map<std::string, int> mPeriodIndex;
};
#endif