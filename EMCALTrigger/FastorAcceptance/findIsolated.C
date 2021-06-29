#include "../../meta/stl.C"
#include "../../meta/root.C"
#ifndef __CLING_
#include "AliCDBManager.h"
#include "AliEMCALGeometry.h"

#include "AliEmcalTriggerMaskHandlerOCDB.h"
#endif

enum class FastORStatus_t {
    ISOLATED,
    GROUPED,
    UNKNOWN
};

int getYear(int runnumber) {
    std::map<int, std::pair<int, int>> runranges = {
        {2015, {208402, 247167}},
        {2016, {247656, 267252}},
        {2017, {267402, 282843}},
        {2018, {282908, 297635}}
    };
    int result = -1;
    for(auto [year, range] : runranges) {
        if(runnumber >= range.first && runnumber <= range.second) {
            result = year;
            break;
        }
    }
    return result;
}

void findIsolated(int runnumber) {
    const int ROWMIN_EMCAL = 0,
              ROWMAX_EMCAL = 63,
              ROWMIN_DCAL = 64,
              ROWMAX_DCAL = 103,
              COLMIN = 0,
              COLMAX = 47;
	auto year = getYear(runnumber);	
    if(year < 0) {
        std::cerr << "Run " << runnumber << " no a valid run2 runnumber, cannot process ..." << std::endl;
        return;
    }

	auto cdb = AliCDBManager::Instance();
	cdb->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", year));

	auto geo = AliEMCALGeometry::GetInstanceFromRunNumber(runnumber);
    auto triggermapping = geo->GetTriggerMapping();

    std::map<int, FastORStatus_t> maskedFastorsL0;
    auto masked = PWG::EMCAL::AliEmcalTriggerMaskHandlerOCDB::Instance()->GetMaskedFastorIndicesL0(runnumber);
    std::cout << "Found " << masked.size() << " FastORs" << std::endl;
    for(auto channel : masked) {
        if(maskedFastorsL0.find(channel) != maskedFastorsL0.end()){
            // FastOr already listed (because it is a neighbour of 
            // a channel that has already been checked)
            continue;
        }
        int col, row;
        triggermapping->GetPositionInEMCALFromAbsFastORIndex(channel, col, row);
        bool isEMCAL = row <= ROWMAX_EMCAL;
        int rowmin, rowmax, 
            colmin = (col-1) >= COLMIN ? col-1 : col, 
            colmax = (col+1) <= COLMAX ? col+1 : col;
        // Rows in the other subdetector are not considered neighbor even if the 
        // row index is the next
        if(isEMCAL) {
            // EMCAL
            rowmin = (row-1) >= ROWMIN_EMCAL ? row-1 : row;
            rowmax = (row+1) <= ROWMAX_EMCAL ? row+1 : row;
        } else {
            // DCAL
            rowmin = (row-1) >= ROWMIN_EMCAL ? row-1 : row;
            rowmax = (row+1) <= ROWMAX_EMCAL ? row+1 : row;
        }

        // find direct masked neighbors (including diagonal ones
        // as they can end up in the same patch)
        std::vector<int> maskedNeighbors;
        for(int irow = rowmin; irow <= rowmax; irow += 1) {
            for(int icol = colmin; icol <= colmax; icol += 1) {
                if(col == icol && row == irow)          // Exclude own position, otherwise the check will be always true
                    continue;
                int absfastor = -1;
                triggermapping->GetAbsFastORIndexFromPositionInEMCAL(icol, irow, absfastor);
                auto found = std::find(masked.begin(), masked.end(), absfastor) != masked.end(); 
                if(found)
                    maskedNeighbors.push_back(absfastor);
            }
        }

        FastORStatus_t currentstatus = FastORStatus_t::UNKNOWN;
        if(maskedNeighbors.size()) {
            currentstatus = FastORStatus_t::GROUPED;

            // Mask all neighbours as they have at least this channel as neighbor
            for(auto neighbor : maskedNeighbors) {
                if(maskedFastorsL0.find(neighbor) == maskedFastorsL0.end()) 
                    maskedFastorsL0[neighbor] = FastORStatus_t::GROUPED;
            }
        } else {
            currentstatus = FastORStatus_t::ISOLATED;
        }
        maskedFastorsL0[channel] = currentstatus;
    }

    std::set<int> isolatedFastORs, groupedFastORs;
    for(auto [fastORID, status] : maskedFastorsL0) {
        switch(status) {
            case FastORStatus_t::ISOLATED: isolatedFastORs.insert(fastORID); break;
            case FastORStatus_t::GROUPED: groupedFastORs.insert(fastORID); break;
            case FastORStatus_t::UNKNOWN: std::cout << "FastOR " << fastORID << " with unknown status" << std::endl; break;
        };
    }

    std::string filenameIsolated = "fastORsIsolated_" + std::to_string(runnumber) + ".txt",
                filenameGrouped = "fastORsGrouped_"  + std::to_string(runnumber) + ".txt";

    std::ofstream fileisolated(filenameIsolated.data()), filegrouped(filenameGrouped.data());
    for(auto fastORID : isolatedFastORs) fileisolated << fastORID << std::endl;
    for(auto fastORID : groupedFastORs) filegrouped << fastORID << std::endl;
    fileisolated.close();
    filegrouped.close();
}