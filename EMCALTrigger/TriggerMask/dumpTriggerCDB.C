#include "../../meta/root.C"
#include "../../helpers/cdb.C"

#ifndef __CLING__
#include "AliCDBManager.h"
#include "AliCDBEntry.h"
#include "AliCDBPath.h"
#include "AliEMCALTriggerDCSConfig.h"
#include "AliEMCALTriggerSTUDCSConfig.h"
#include "AliEMCALTriggerTRUDCSConfig.h"
#endif

void dumpTriggerCDB(int runnumber) {
	auto year = getYearForRunNumber(runnumber);
	auto cdb = AliCDBManager::Instance();
	cdb->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", year));
	cdb->SetRun(runnumber);

	auto triggerconfg = static_cast<AliEMCALTriggerDCSConfig *>(cdb->Get("EMCAL/Calib/Trigger")->GetObject());
	auto emcalstu = triggerconfg->GetSTUDCSConfig(false),
	     dcalstu = triggerconfg->GetSTUDCSConfig(true);

	std::cout << "EMCAL STU Config:" << std::endl;
	std::cout << "==============================" << std::endl;
	std::cout << *emcalstu << std::endl << std::endl;

	std::cout << "DCAL STU Config:" << std::endl;
	std::cout << "==============================" << std::endl;
	std::cout << *dcalstu << std::endl << std::endl;

	std::cout << "TRU Configs:" << std::endl;
	std::cout << "==============================" << std::endl;
	
	auto truarray = triggerconfg->GetTRUArr();
	for(int itru = 0; itru < truarray->GetEntries(); itru++){
		auto tru = static_cast<AliEMCALTriggerTRUDCSConfig *>(truarray->At(itru));
		std::cout << "TRU " << itru << " Config:" << std::endl;
		std::cout << "------------------------------" << std::endl;
		std::cout << *tru << std::endl;
		std::cout << "------------------------------" << std::endl;
	}
}
