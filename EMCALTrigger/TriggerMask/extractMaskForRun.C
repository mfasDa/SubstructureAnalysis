#ifndef __CLING__
#include "TTree.h"
#include "AliCDBManager.h"
#include "AliEMCALGeometry.h"
#include "AliEmcalTriggerMaskHandlerOCDB.h"
#endif

#include "../../helpers/triggerRawDB.C"

int getYear(int runnumber) {
	struct rangeentry {
		int year;
		int runmin;
		int runmax;
	};
	std::vector<rangeentry> ranges = {{2016, 247656, 267252}, 
				          {2017, 267402, 282843}, 
					  {2018, 282908, 297635}};
	int year = -1;
	auto result = std::find_if(ranges.begin(), ranges.end(), [runnumber](const rangeentry &en) { return runnumber >= en.runmin && runnumber <= en.runmax; });
	if(result != ranges.end()) {
		year = result->year;
	}
	return year;
}

struct treedata {
	int runnumber;
	int eventsL0;
	int eventsG1;
	int nmaskL0;
	int nmaskL1;
};

void extractMask(int run) {
	int year = getYear(run);
	if(year == -1) {
		printf("Run %d not from 2016 - 2018\n", year);
		return;
	}

	TriggerRawDB triggerdb;
	triggerdb.buildForYear(year);
	bool hasrun = triggerdb.hasRun(run);
	if(!hasrun) {
		std::cout << "No number entry in trigger db found for run " << run << ", no possible to store events" << std::endl;
	}

	AliCDBManager *mgr = AliCDBManager::Instance();
	mgr->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%s/OCDB", year));
	mgr->SetRun(run);

	AliEMCALGeometry::GetInstanceFromRunNumber(run);

	auto maskhandler = PWG::EMCAL::AliEmcalTriggerMaskHandlerOCDB::Instance();
	auto maskedL0 = maskhandler->GetMaskedFastorIndicesL0(),
	     maskedL1 = maskhandler->GetMaskedFastorIndicesL1();

	treedata result;
	result.runnumber = run;
	result.nmaskL0 = maskedL0.size();
	result.nmaskL1 = maskedL1.size();
	if(hasrun) {
		result.eventsL0 = triggerdb.getTriggersForRun(run, "CEMC7");
		result.eventsG1 = triggerdb.getTriggersForRun(run, "EG1");
	} else {
		result.eventsL0 = 0;
		result.eventsG1 = 0;
	}

	std::unique_ptr<TFile> outputfile(TFile::Open("triggermask.root", "RECREATE"));
	TTree *outputtree = new TTree("triggermask", "Tree with trigger mask");
	outputtree->Branch("triggermask", &result, "runnumber:eventsL0:eventsG1:nmaskL0:nmaskL1/I");
	outputtree->Fill();

}