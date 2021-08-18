#ifndef __CLING__
#include "TTree.h"
#include "AliCDBManager.h"
#include "AliEMCALGeometry.h"
#include "AliEmcalTriggerMaskHandlerOCDB.h"
#endif

#include "../../helpers/cdb.C"
#include "../../helpers/triggerRawDB.C"

struct triggermask {
	int runnumber;
	int year;
	int eventsL0;
	int eventsG1;
	int nmaskL0ALL;
	int nmaskL0EMCAL;
	int nmaskL0DCAL;	
	int nmaskL1ALL;
	int nmaskL1EMCAL;
	int nmaskL1DCAL;
	char period[512];
};

std::map<std::string, int> filterDetector(std::vector<int> fastorIDs){
	int nEMCAL = 0, nDCAL = 0;
	auto geo = AliEMCALGeometry::GetInstance();
	for(int fastOR: fastorIDs) {
		int cellIDs[4];
		geo->GetCellIndexFromFastORIndex(fastOR, cellIDs);
		auto sm = geo->GetSuperModuleNumber(cellIDs[0]);
		if(sm < 12) nEMCAL++;
		else nDCAL++;
	}
	return {{"EMCAL", nEMCAL}, {"DCAL", nDCAL}};
}

void extractMaskForRun(int run) {
	int year = getYearForRunNumber(run);
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
	mgr->SetDefaultStorage(Form("local:///cvmfs/alice-ocdb.cern.ch/calibration/data/%d/OCDB", year));
	mgr->SetRun(run);

	AliEMCALGeometry::GetInstanceFromRunNumber(run);

	auto maskhandler = PWG::EMCAL::AliEmcalTriggerMaskHandlerOCDB::Instance();
	auto maskedL0 = maskhandler->GetMaskedFastorIndicesL0(run),
	     maskedL1 = maskhandler->GetMaskedFastorIndicesL1(run);

	auto filteredL0 = filterDetector(maskedL0),
	     filteredL1 = filterDetector(maskedL1);

	triggermask result;
	result.runnumber = run;
	result.year = year;
	result.nmaskL0ALL = maskedL0.size();
	result.nmaskL0EMCAL = filteredL0["EMCAL"];
	result.nmaskL0DCAL = filteredL0["DCAL"];
	result.nmaskL1ALL = maskedL1.size();
	result.nmaskL1EMCAL = filteredL1["EMCAL"];
	result.nmaskL1DCAL = filteredL1["DCAL"];
	if(hasrun) {
		std::string period = triggerdb.getPeriod(run);
		std::cout << "Found run " << run << " ("<< period <<") in trigger DB" << std::endl;
		strcpy(result.period, period.data());
		result.eventsL0 = triggerdb.getTriggersForRun(run, "CEMC7");
		result.eventsG1 = triggerdb.getTriggersForRun(run, "EG1");
	} else {
		std::cout << "No run " << run << " in trigger DB - storing zeros" << std::endl;
		result.eventsL0 = 0;
		result.eventsG1 = 0;
	}

	std::unique_ptr<TFile> outputfile(TFile::Open("triggermask.root", "RECREATE"));
	TTree *outputtree = new TTree("triggermask", "Tree with trigger mask");
	outputtree->Branch("triggermask", &result, "runnumber/I:year:eventsL0:eventsG1:nmaskL0ALL:nmaskL0EMCAL:nmaskL0DCAL:nmaskL1ALL:nmaskL1EMCAL:nmaskL1DCAL:period/C");
	outputtree->Fill();
	outputtree->Print();
	outputtree->Write();

}