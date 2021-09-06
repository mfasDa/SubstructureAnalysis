#include "../../meta/root.C"

void getMinMaxMeanMasked(const char *filename) {
	const int NFASTORTRU = 96,
	          NTRUEMCAL = 3 * 10 + 1 * 2,
		  NTRUDCAL = 2 * 6 + 1 * 2,
	          NFASTOREMCAL = NFASTORTRU * NTRUEMCAL,
	          NFASTORDCAL = NFASTORTRU * NTRUDCAL;
	auto frame = ROOT::RDataFrame("triggermask", filename);

	auto runsnullasked = frame.Filter("triggermask.nmaskL0ALL == 0"),
	     runsfulldcalmask = frame.Filter(Form("triggermask.nmaskL1DCAL == %d", NFASTORDCAL)),
	     runsfullemcalmask = frame.Filter(Form("triggermask.nmaskL1EMCAL == %d", NFASTOREMCAL));

	std::cout << "Runs with no mask: " << std::endl;
	runsnullasked.Foreach([](int run) { std::cout << run << std::endl; }, {"triggermask.runnumber"});

	std::cout << "Runs with full DCAL mask" << std::endl;
	runsfullemcalmask.Foreach([](int run) { std::cout << run << std::endl; }, {"triggermask.runnumber"});

	std::cout << "Runs with full DCAL mask" << std::endl;
	runsfulldcalmask.Foreach([](int run) { std::cout << run << std::endl; }, {"triggermask.runnumber"});

	auto runsfulldcalmasked = frame.Filter("triggermask.nmaskL0ALL > 0").Filter(Form("triggermask.nmaskL1DCAL < %d", NFASTORDCAL));
	auto 
	     minL0ALL = runsfulldcalmasked.Min("triggermask.nmaskL0ALL"),
	     minL0EMCAL = runsfulldcalmasked.Min("triggermask.nmaskL0EMCAL"),
	     minL0DCAL = runsfulldcalmasked.Min("triggermask.nmaskL0DCAL"),
	     minL1ALL = runsfulldcalmasked.Min("triggermask.nmaskL1ALL"),
	     minL1EMCAL = runsfulldcalmasked.Min("triggermask.nmaskL1EMCAL"),
	     minL1DCAL = runsfulldcalmasked.Min("triggermask.nmaskL1DCAL"),
	     maxL0ALL = runsfulldcalmasked.Max("triggermask.nmaskL0ALL"),
	     maxL0EMCAL = runsfulldcalmasked.Max("triggermask.nmaskL0EMCAL"),
	     maxL0DCAL = runsfulldcalmasked.Max("triggermask.nmaskL0DCAL"),
	     maxL1ALL = runsfulldcalmasked.Max("triggermask.nmaskL1ALL"),
	     maxL1EMCAL = runsfulldcalmasked.Max("triggermask.nmaskL1EMCAL"),
	     maxL1DCAL = runsfulldcalmasked.Max("triggermask.nmaskL1DCAL");

	double  minAL0 = double(*minL0ALL)/double(NFASTOREMCAL+NFASTORDCAL) * 100.,
		minEL0 = double(*minL0EMCAL)/double(NFASTOREMCAL) * 100.,
		minDL0 = double(*minL0DCAL)/double(NFASTORDCAL) * 100.,
	        minAL1 = double(*minL1ALL)/double(NFASTOREMCAL+NFASTORDCAL) * 100.,
		minEL1 = double(*minL1EMCAL)/double(NFASTOREMCAL) * 100.,
		minDL1 = double(*minL1DCAL)/double(NFASTORDCAL) * 100.,
		maxAL0 = double(*maxL0ALL)/double(NFASTOREMCAL+NFASTORDCAL) * 100.,
		maxEL0 = double(*maxL0EMCAL)/double(NFASTOREMCAL) * 100.,
		maxDL0 = double(*maxL0DCAL)/double(NFASTORDCAL) * 100.,
	        maxAL1 = double(*maxL1ALL)/double(NFASTOREMCAL+NFASTORDCAL) * 100.,
		maxEL1 = double(*maxL1EMCAL)/double(NFASTOREMCAL) * 100.,
		maxDL1 = double(*maxL1DCAL)/double(NFASTORDCAL) * 100.;



	std::cout <<  "|   Type |   All   |  EMCAL  |   DCAL  |" << std::endl
	          <<  "|========|=========|=========|=========|" << std::setprecision(2) << std::setw(6) << std::endl
		  <<  "| Min L0 |  " << minAL0 << " |  " << minEL0 << " |  " << minDL0 << " |" << std::endl
		  <<  "| Max L0 |  " << maxAL0 << " |  " << maxEL0 << " |  " << maxDL0 << " |" << std::endl
		  <<  "| Min L1 |  " << minAL1 << " |  " << minEL1 << " |  " << minDL1 << " |" << std::endl
		  <<  "| Max L1 |  " << maxAL1 << " |  " << maxEL1 << " |  " << maxDL1 << " |" << std::endl;
}