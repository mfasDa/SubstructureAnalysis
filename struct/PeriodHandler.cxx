#ifndef PERIODHANDLER_H
#define PERIODHANDLER_H

#include "../meta/stl.C"

class PeriodHandler {
    public:
        struct PeriodData {
            std::string name;
            int year;
            int minrun;
            int maxrun;        

            bool operator==(const PeriodData &other) const { return minrun == other.minrun && maxrun == other.maxrun; }
            bool operator<(const PeriodData &other) const { return maxrun < other.minrun; }
        };

        class PeriodNotFoundException : public std::exception{
        public:
            PeriodNotFoundException(int run) : fRun(run), fMessage() {
                std::stringstream msgbuilder;
                msgbuilder << "No period found for run " << fRun;
                fMessage = msgbuilder.str();
            }
            ~PeriodNotFoundException() noexcept = default;

            const char *what() const noexcept { return fMessage.data(); }
            int getRun() const { return fRun; }
        private:
            int fRun;
            std::string fMessage;
        };

        PeriodHandler() { build(); }
        ~PeriodHandler() = default;

        const PeriodData &findPeriod(int run) const { 
            auto found = std::find_if(fPeriods.begin(), fPeriods.end(), [run](const PeriodData &test) { return run >= test.minrun && run <= test.maxrun; });
            if(found == fPeriods.end()) throw PeriodNotFoundException(run);
            return *found;
        }

        int getYearForRun(int run) const {
            auto &period = findPeriod(run);
            return period.year;
        }

        std::string getPeriodName(int run) const {
            auto &period = findPeriod(run);
            return period.name;
        }

        std::set<PeriodData> getListOfPeriods(int minrun, int maxrun) { 
            std::set<PeriodData> result;
            for(const auto &per : fPeriods) {
                if((per.minrun >= minrun && per.minrun <= maxrun) || (per.maxrun >= minrun && per.maxrun <= maxrun)) result.insert(per);
            }
            return result;
        };

    private:
        void build() {
            fPeriods.insert({"LHC17h", 2017, 271839, 273103});
            fPeriods.insert({"LHC17i", 2017, 273486, 274442});
            fPeriods.insert({"LHC17j", 2017, 274591, 274671});
            fPeriods.insert({"LHC17k", 2017, 274690, 276508});
            fPeriods.insert({"LHC17l", 2017, 276551, 278729});
            fPeriods.insert({"LHC17m", 2017, 278818, 280140});
            fPeriods.insert({"LHC17o", 2017, 280282, 281961});
            fPeriods.insert({"LHC17r", 2017, 282504, 282704});
            fPeriods.insert({"LHC18d", 2018, 285978, 286350});
            fPeriods.insert({"LHC18e", 2018, 286380, 286958});
            fPeriods.insert({"LHC18f", 2018, 286982, 287977});
            fPeriods.insert({"LHC18g", 2018, 288619, 288750});
            fPeriods.insert({"LHC18h", 2018 ,288804, 288806});
            fPeriods.insert({"LHC18i", 2018, 288861, 288909});
            fPeriods.insert({"LHC18j", 2018, 288943, 288943});
            fPeriods.insert({"LHC18k", 2018, 289165, 289201});
            fPeriods.insert({"LHC18l", 2018, 289240, 289971});
            fPeriods.insert({"LHC18m", 2018, 290167, 292839});
            fPeriods.insert({"LHC18n", 2018, 293357, 293362});
            fPeriods.insert({"LHC18o", 2018, 293368, 293898});
            fPeriods.insert({"LHC18p", 2018, 294009, 295232});
            std::cout << "PeriodHandler: Found " << fPeriods.size() << " periods" << std::endl;
        }

        std::set<PeriodData> fPeriods;
};

#endif