#include "../../meta/root.C"

struct branchnames {
    std::string mtreename = "triggermask";
    std::string mrunnumber = "runnumber";
    std::string myear = "year";
    std::string meventsEL0 = "eventsEL0";
    std::string meventsEG1 = "eventsEG1"; 
    std::string meventsDL0 = "eventsDL0";
    std::string meventsDG1 = "eventsDG1"; 
    std::string mnmaskL0ALL = "nmaskL0ALL";
    std::string mnmaskL0EMCAL = "nmaskL0EMCAL";
    std::string mnmaskL0DCAL = "nmaskL0DCAL";
    std::string mnmaskL1ALL = "nmaskL1ALL";
    std::string mnmaskL1EMCAL = "nmaskL1EMCAL";
    std::string mnmaskL1DCAL = "nmaskL1DCAL";
    std::string mperiod = "period";

    // dynamic columns
    std::string mnmaskL0ALLFrac = "nmaskL0ALLFrac";
    std::string mnmaskL0EMCALFrac = "nmaskL0EMCALFrac";
    std::string mnmaskL0DCALFrac = "nmaskL0DCALFrac";
    std::string mnmaskL1ALLFrac = "nmaskL1ALLFrac";
    std::string mnmaskL1EMCALRelFrac = "nmaskL1EMCALFrac";
    std::string mnmaskL1DCALRelFrac = "nmaskL1DCALFrac";
    std::string mfracEventsEL0All = "fracEventsEL0All";
    std::string mfracEventsEG1All = "fracEventsEG1All";
    std::string mfracEventsDL0All = "fracEventsDL0All";
    std::string mfracEventsDG1All = "fracEventsDG1All";
    std::string mfracEventsEL0Filter = "fracEventsEL0Filter";
    std::string mfracEventsEG1Filter = "fracEventsEG1Filter";
    std::string mfracEventsDL0Filter = "fracEventsDL0Filter";
    std::string mfracEventsDG1Filter = "fracEventsDG1Filter";

    std::string runnumber() const { return mtreename + "." + mrunnumber; }
    std::string year() const { return mtreename + "." + myear; }
    std::string eventsEL0() const { return mtreename + "." + meventsEL0; }
    std::string eventsEG1() const { return mtreename + "." + meventsEG1; }
    std::string eventsDL0() const { return mtreename + "." + meventsDL0; }
    std::string eventsDG1() const { return mtreename + "." + meventsDG1; }
    std::string nmaskL0ALL() const { return mtreename + "." + mnmaskL0ALL; }
    std::string nmaskL0EMCAL() const { return mtreename + "." + mnmaskL0EMCAL; }
    std::string nmaskL0DCAL() const { return mtreename + "." + mnmaskL0DCAL; }
    std::string nmaskL1ALL() const { return mtreename + "." + mnmaskL1ALL; }
    std::string nmaskL1EMCAL() const { return mtreename + "." + mnmaskL1EMCAL; }
    std::string nmaskL1DCAL() const { return mtreename + "." + mnmaskL1DCAL; }
    std::string period() const { return mtreename + "." + mperiod; }

    // dynamic columns (treename needs to be dropped in this case)
    std::string fracmaskL0ALL() const { return mnmaskL0ALLFrac; }
    std::string fracmaskL0EMCAL() const { return mnmaskL0EMCALFrac; }
    std::string fracmaskL0DCAL() const { return mnmaskL0DCALFrac; }
    std::string fracmaskL1ALL() const { return mnmaskL1ALLFrac; }
    std::string fracmaskL1EMCAL() const { return mnmaskL1EMCALRelFrac; }
    std::string fracmaskL1DCAL() const { return mnmaskL1DCALRelFrac; }
    std::string fracEventsEL0All() const { return mfracEventsEL0All; }
    std::string fracEventsEG1All() const { return mfracEventsEG1All; }
    std::string fracEventsDL0All() const { return mfracEventsDL0All; }
    std::string fracEventsDG1All() const { return mfracEventsDG1All; }
    std::string fracEventsEL0Filter() const { return mfracEventsEL0Filter; }
    std::string fracEventsEG1Filter() const { return mfracEventsEG1Filter; }
    std::string fracEventsDL0Filter() const { return mfracEventsDL0Filter; }
    std::string fracEventsDG1Filter() const { return mfracEventsDG1Filter; }
}; 

template<typename T>
std::string compare(std::string_view branchname, T value, std::string comparator) {
    std::stringstream cmdbuilder;
    cmdbuilder << branchname << " " << comparator << " " << value;
    return cmdbuilder.str();
}

template<typename T>
std::string equals(std::string_view branchname, T value) {
    return compare(branchname, value, "==");
}

template<typename T>
std::string smaller(std::string_view branchname, T value) {
    return compare(branchname, value, "<");
}

template<typename T>
std::string larger(std::string_view branchname, T value) {
    return compare(branchname, value, ">");
}

template<typename T>
std::string scale(std::string_view branchname, T value) {
    std::stringstream cmdbuilder;
    if(std::is_floating_point<T>::value){
        cmdbuilder << "double(" <<branchname << ")";
    } else {
        cmdbuilder << branchname;
    }
    cmdbuilder << "/" << value;
    return cmdbuilder.str();
}

void getMinMaxMeanMasked(const char *filename) {
	const int NFASTORTRU = 96,
	          NTRUEMCAL = 3 * 10 + 1 * 2,
		      NTRUDCAL = 2 * 6 + 1 * 2,
	          NFASTOREMCAL = NFASTORTRU * NTRUEMCAL,
	          NFASTORDCAL = NFASTORTRU * NTRUDCAL,
              NFASTORALL = NFASTOREMCAL + NFASTORDCAL;
	auto frame = ROOT::RDataFrame("triggermask", filename);

    branchnames branchhandler;
	auto runsnullasked = frame.Filter(equals(branchhandler.nmaskL0ALL(), 0)),
	     runsfulldcalmask = frame.Filter(equals(branchhandler.nmaskL1DCAL(), NFASTORDCAL)),
	     runsfullemcalmask = frame.Filter(equals(branchhandler.nmaskL1EMCAL(), NFASTOREMCAL));

	std::cout << "Runs with no mask: " << std::endl;
	runsnullasked.Foreach([](int run) { std::cout << run << std::endl; }, { branchhandler.runnumber() });

	std::cout << "Runs with full EMCAL mask: " << std::endl;
	runsfullemcalmask.Foreach([](int run) { std::cout << run << std::endl; }, { branchhandler.runnumber() });

	std::cout << "Runs with full DCAL mask: " << std::endl;
	runsfulldcalmask.Foreach([](int run) { std::cout << run << std::endl; }, { branchhandler.runnumber() });
    
    auto sumEL0All = *frame.Sum(branchhandler.eventsEL0()),
         sumEG1All = *frame.Sum(branchhandler.eventsEG1()),
         sumDL0All = *frame.Sum(branchhandler.eventsDL0()),
         sumDG1All = *frame.Sum(branchhandler.eventsDG1());

    // define colums with relative amount of masked cells and relative amount of events
    auto extframe = frame.Define(branchhandler.fracmaskL0ALL(), scale(branchhandler.nmaskL0ALL(), double(NFASTORALL)))
                         .Define(branchhandler.fracmaskL0EMCAL(), scale(branchhandler.nmaskL0EMCAL(), double(NFASTOREMCAL)))
                         .Define(branchhandler.fracmaskL0DCAL(), scale(branchhandler.nmaskL0DCAL(), double(NFASTORDCAL)))
                         .Define(branchhandler.fracmaskL1ALL(), scale(branchhandler.nmaskL1ALL(), double(NFASTORALL)))
                         .Define(branchhandler.fracmaskL1EMCAL(), scale(branchhandler.nmaskL1EMCAL(), double(NFASTOREMCAL)))
                         .Define(branchhandler.fracmaskL1DCAL(), scale(branchhandler.nmaskL1DCAL(), double(NFASTORDCAL)))
                         .Define(branchhandler.fracEventsEL0All(), scale(branchhandler.eventsEL0(), double(sumEL0All)))
                         .Define(branchhandler.fracEventsEG1All(), scale(branchhandler.eventsEG1(), double(sumEG1All)))
                         .Define(branchhandler.fracEventsDL0All(), scale(branchhandler.eventsDL0(), double(sumDL0All)))
                         .Define(branchhandler.fracEventsDG1All(), scale(branchhandler.eventsDG1(), double(sumDG1All)));
    
    // Time series EMCAL/DCAL/EMCAL+DCAL L0 and L1 before filtering
    auto trendEventsEL0AbsAll= extframe.Graph(branchhandler.runnumber(), branchhandler.eventsEL0()),
         trendEventsEG1AbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.eventsEG1()),
         trendEventsDL0AbsAll= extframe.Graph(branchhandler.runnumber(), branchhandler.eventsDL0()),
         trendEventsDG1AbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.eventsDG1()),
         trendEventsEL0RelAll= extframe.Graph(branchhandler.runnumber(), branchhandler.fracEventsEL0All()),
         trendEventsEG1RelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracEventsEG1All()),
         trendEventsDL0RelAll= extframe.Graph(branchhandler.runnumber(), branchhandler.fracEventsDL0All()),
         trendEventsDG1RelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracEventsDG1All()),
         trendMaskL0AllAbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.nmaskL0ALL()),
         trendMaskL0EMCALAbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.nmaskL0EMCAL()),
         trendMaskL0DCALAbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.nmaskL0DCAL()),
         trendMaskL1AllAbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.nmaskL0ALL()),
         trendMaskL1EMCALAbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.nmaskL0EMCAL()),
         trendMaskL1DCALAbsAll = extframe.Graph(branchhandler.runnumber(), branchhandler.nmaskL0DCAL()),
         trendMaskL0AllRelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0ALL()),
         trendMaskL0EMCALRelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0EMCAL()),
         trendMaskL0DCALRelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0DCAL()),
         trendMaskL1AllRelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0ALL()),
         trendMaskL1EMCALRelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0EMCAL()),
         trendMaskL1DCALRelAll = extframe.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0DCAL());

	auto runsfulldcalmasked = extframe.Filter(larger(branchhandler.nmaskL0ALL(), 0)).Filter(smaller(branchhandler.nmaskL1DCAL(), NFASTORDCAL));

    auto sumEL0Filter = *runsfulldcalmasked.Sum(branchhandler.eventsEL0()),
         sumEG1Filter = *runsfulldcalmasked.Sum(branchhandler.eventsEG1()),
         sumDL0Filter = *runsfulldcalmasked.Sum(branchhandler.eventsDL0()),
         sumDG1Filter = *runsfulldcalmasked.Sum(branchhandler.eventsDG1());

    auto extrunsfulldcalmasked = runsfulldcalmasked.Define(branchhandler.fracEventsEL0Filter(), scale(branchhandler.eventsEL0(), sumEL0Filter))
                                                   .Define(branchhandler.fracEventsEG1Filter(), scale(branchhandler.eventsEG1(), sumEG1Filter))
                                                   .Define(branchhandler.fracEventsDL0Filter(), scale(branchhandler.eventsDL0(), sumDL0Filter))
                                                   .Define(branchhandler.fracEventsDG1Filter(), scale(branchhandler.eventsDG1(), sumDG1Filter));

    // Time series EMCAL/DCAL/EMCAL+DCAL L0 and L1 before filtering
    auto trendEventsEL0AbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.eventsEL0()),
         trendEventsEG1AbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.eventsEG1()),
         trendEventsDL0AbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.eventsDL0()),
         trendEventsDG1AbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.eventsDG1()),
         trendEventsEL0RelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracEventsEL0Filter()),
         trendEventsEG1RelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracEventsEG1Filter()),
         trendEventsDL0RelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracEventsDL0Filter()),
         trendEventsDG1RelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracEventsDG1Filter()),
         trendMaskL0AllAbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.nmaskL0ALL()),
         trendMaskL0EMCALAbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.nmaskL0EMCAL()),
         trendMaskL0DCALAbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.nmaskL0DCAL()),
         trendMaskL1AllAbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.nmaskL0ALL()),
         trendMaskL1EMCALAbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.nmaskL0EMCAL()),
         trendMaskL1DCALAbsFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.nmaskL0DCAL()),
         trendMaskL0AllRelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0ALL()),
         trendMaskL0EMCALRelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0EMCAL()),
         trendMaskL0DCALRelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0DCAL()),
         trendMaskL1AllRelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0ALL()),
         trendMaskL1EMCALRelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0EMCAL()),
         trendMaskL1DCALRelFilter = extrunsfulldcalmasked.Graph(branchhandler.runnumber(), branchhandler.fracmaskL0DCAL());

	double minAL0 = double(*extrunsfulldcalmasked.Min(branchhandler.fracmaskL0ALL())) * 100.,
		   minEL0 = double(*extrunsfulldcalmasked.Min(branchhandler.fracmaskL0EMCAL())) * 100.,
		   minDL0 = double(*extrunsfulldcalmasked.Min(branchhandler.fracmaskL0DCAL())) * 100.,
	       minAL1 = double(*extrunsfulldcalmasked.Min(branchhandler.fracmaskL1ALL())) * 100.,
		   minEL1 = double(*extrunsfulldcalmasked.Min(branchhandler.fracmaskL1EMCAL())) * 100.,
		   minDL1 = double(*extrunsfulldcalmasked.Min(branchhandler.fracmaskL1DCAL())) * 100.,
		   maxAL0 = double(*extrunsfulldcalmasked.Max(branchhandler.fracmaskL0ALL())) * 100.,
		   maxEL0 = double(*extrunsfulldcalmasked.Max(branchhandler.fracmaskL0EMCAL())) * 100.,
		   maxDL0 = double(*extrunsfulldcalmasked.Max(branchhandler.fracmaskL0DCAL())) * 100.,
	       maxAL1 = double(*extrunsfulldcalmasked.Max(branchhandler.fracmaskL1ALL())) * 100.,
		   maxEL1 = double(*extrunsfulldcalmasked.Max(branchhandler.fracmaskL1EMCAL())) * 100.,
		   maxDL1 = double(*extrunsfulldcalmasked.Max(branchhandler.fracmaskL1DCAL())) * 100.;

    // Calculate mean: As it is weighted with the relative fraction of events
    // Mean function from TMath must be used, togther with colums of relative
    // fractions of masked channels and events
    auto colEvEL0 = extrunsfulldcalmasked.Take<double>(branchhandler.fracEventsEL0Filter()),
         colEvDL0 = extrunsfulldcalmasked.Take<double>(branchhandler.fracEventsDL0Filter()),
         colEvEG1 = extrunsfulldcalmasked.Take<double>(branchhandler.fracEventsEL0Filter()),
         colEvDG1 = extrunsfulldcalmasked.Take<double>(branchhandler.fracEventsDL0Filter());
    auto fracMaskedL0All = extrunsfulldcalmasked.Take<double>(branchhandler.fracmaskL0ALL()),
         fracMaskedL0EMCAL = extrunsfulldcalmasked.Take<double>(branchhandler.fracmaskL0EMCAL()),
         fracMaskedL0DCAL = extrunsfulldcalmasked.Take<double>(branchhandler.fracmaskL0DCAL()),
         fracMaskedL1All = extrunsfulldcalmasked.Take<double>(branchhandler.fracmaskL0ALL()),
         fracMaskedL1EMCAL = extrunsfulldcalmasked.Take<double>(branchhandler.fracmaskL0EMCAL()),
         fracMaskedL1DCAL = extrunsfulldcalmasked.Take<double>(branchhandler.fracmaskL0DCAL());

    double meanAL0 = TMath::Mean(fracMaskedL0All.begin(), fracMaskedL0All.end(), colEvEL0.begin()) * 100.,
           meanEL0 = TMath::Mean(fracMaskedL0EMCAL.begin(), fracMaskedL0EMCAL.end(), colEvEL0.begin()) * 100.,
           meanDL0 = TMath::Mean(fracMaskedL0DCAL.begin(), fracMaskedL0DCAL.end(), colEvDL0.begin()) * 100.,
           meanAL1 = TMath::Mean(fracMaskedL1All.begin(), fracMaskedL1All.end(), colEvEG1.begin()) * 100.,
           meanEL1 = TMath::Mean(fracMaskedL1EMCAL.begin(), fracMaskedL1EMCAL.end(), colEvEG1.begin()) * 100.,
           meanDL1 = TMath::Mean(fracMaskedL1DCAL.begin(), fracMaskedL1DCAL.end(), colEvDG1.begin()) * 100.;


    // Printout number min / max / mean
	std::cout <<  "|  Type   |   All   |  EMCAL  |   DCAL  |" << std::endl
	          <<  "|=========|=========|=========|=========|" << std::setprecision(2) << std::setw(6) << std::endl
		      <<  "| Min L0  |  " << std::setw(6) << std::setfill(' ') << minAL0 << " |  " << std::setw(6) << std::setfill(' ') << minEL0 << " |  " << std::setw(6) << std::setfill(' ') << minDL0 << " |" << std::endl
		      <<  "| Max L0  |  " << std::setw(6) << std::setfill(' ') << maxAL0 << " |  " << std::setw(6) << std::setfill(' ') << maxEL0 << " |  " << std::setw(6) << std::setfill(' ') << maxDL0 << " |" << std::endl
		      <<  "| Mean L0 |  " << std::setw(6) << std::setfill(' ') << meanAL0 << " |  " << std::setw(6) << std::setfill(' ') << meanEL0 << " |  " << std::setw(6) << std::setfill(' ') << meanDL0 << " |" << std::endl
		      <<  "| Min L1  |  " << std::setw(6) << std::setfill(' ') << minAL1 << " |  " << std::setw(6) << std::setfill(' ') << minEL1 << " |  " << std::setw(6) << std::setfill(' ') << minDL1 << " |" << std::endl
		      <<  "| Max L1  |  " << std::setw(6) << std::setfill(' ') << maxAL1 << " |  " << std::setw(6) << std::setfill(' ') << maxEL1 << " |  " << std::setw(6) << std::setfill(' ') << maxDL1 << " |" << std::endl
		      <<  "| Mean L1 |  " << std::setw(6) << std::setfill(' ') << meanAL1 << " |  " << std::setw(6) << std::setfill(' ') << meanEL1 << " |  " << std::setw(6) << std::setfill(' ') << meanDL1 << " |" << std::endl;

    // Sort graphs
    trendEventsEL0AbsAll->Sort();
    trendEventsEG1AbsAll->Sort();
    trendEventsDL0AbsAll->Sort();
    trendEventsDG1AbsAll->Sort();
    trendEventsEL0RelAll->Sort();
    trendEventsEG1RelAll->Sort();
    trendEventsDL0RelAll->Sort();
    trendEventsDG1RelAll->Sort();
    trendMaskL0AllAbsAll->Sort();
    trendMaskL1AllAbsAll->Sort();
    trendMaskL0EMCALAbsAll->Sort();
    trendMaskL1EMCALAbsAll->Sort();
    trendMaskL0DCALAbsAll->Sort();
    trendMaskL1DCALAbsAll->Sort(); 
    trendMaskL0AllRelAll->Sort();
    trendMaskL1AllRelAll->Sort();
    trendMaskL0EMCALRelAll->Sort();
    trendMaskL1EMCALRelAll->Sort();
    trendMaskL0DCALRelAll->Sort();
    trendMaskL1DCALRelAll->Sort(); 
    trendEventsEL0AbsFilter->Sort();
    trendEventsEG1AbsFilter->Sort();
    trendEventsDL0AbsFilter->Sort();
    trendEventsDG1AbsFilter->Sort();
    trendEventsEL0RelFilter->Sort();
    trendEventsEG1RelFilter->Sort();
    trendEventsDL0RelFilter->Sort();
    trendEventsDG1RelFilter->Sort();
    trendMaskL0AllAbsFilter->Sort();
    trendMaskL1AllAbsFilter->Sort();
    trendMaskL0EMCALAbsFilter->Sort();
    trendMaskL1EMCALAbsFilter->Sort();
    trendMaskL0DCALAbsFilter->Sort();
    trendMaskL1DCALAbsFilter->Sort(); 
    trendMaskL0AllRelFilter->Sort();
    trendMaskL1AllRelFilter->Sort();
    trendMaskL0EMCALRelFilter->Sort();
    trendMaskL1EMCALRelFilter->Sort();
    trendMaskL0DCALRelFilter->Sort();
    trendMaskL1DCALRelFilter->Sort(); 

    std::unique_ptr<TFile> trendwriter(TFile::Open("masktrending.root", "RECREATE"));
    trendwriter->mkdir("eventsAll");
    trendwriter->cd("eventsAll");
    trendEventsEL0AbsAll->Write("trendEventsEL0AbsAll");
    trendEventsEG1AbsAll->Write("trendEventsEG1AbsAll");
    trendEventsDL0AbsAll->Write("trendEventsDL0AbsAll");
    trendEventsDG1AbsAll->Write("trendEventsDG1AbsAll");
    trendEventsEL0RelAll->Write("trendEventsEL0RelAll");
    trendEventsEG1RelAll->Write("trendEventsEG1RelAll");
    trendEventsDL0RelAll->Write("trendEventsDL0RelAll");
    trendEventsDG1RelAll->Write("trendEventsDG1RelAll");
    trendwriter->mkdir("eventsFilter");
    trendwriter->cd("eventsFilter");
    trendEventsEL0AbsFilter->Write("trendEventsEL0AbsFilter");
    trendEventsEG1AbsFilter->Write("trendEventsEG1AbsFilter");
    trendEventsDL0AbsFilter->Write("trendEventsDL0AbsFilter");
    trendEventsDG1AbsFilter->Write("trendEventsDG1AbsFilter");
    trendEventsEL0AbsFilter->Write("trendEventsEL0RelFilter");
    trendEventsEG1AbsFilter->Write("trendEventsEG1RelFilter");
    trendEventsDL0AbsFilter->Write("trendEventsDL0RelFilter");
    trendEventsDG1AbsFilter->Write("trendEventsDG1RelFilter");
    trendwriter->mkdir("AbsAll");
    trendwriter->cd("AbsAll");
    trendMaskL0AllAbsAll->Write("trendMaskL0AllAbsAll");
    trendMaskL1AllAbsAll->Write("trendMaskL1AllAbsAll");
    trendMaskL0EMCALAbsAll->Write("trendMaskL0EMCALAbsAll");
    trendMaskL1EMCALAbsAll->Write("trendMaskL1EMCALAbsAll");
    trendMaskL0DCALAbsAll->Write("trendMaskL0DCALAbsAll");
    trendMaskL1DCALAbsAll->Write("trendMaskL1DCALAbsAll");
    trendwriter->mkdir("RelAll");
    trendwriter->cd("RelAll");
    trendMaskL0AllRelAll->Write("trendMaskL0AllRelAll");
    trendMaskL1AllRelAll->Write("trendMaskL1AllRelAll");
    trendMaskL0EMCALRelAll->Write("trendMaskL0EMCALRelAll");
    trendMaskL1EMCALRelAll->Write("trendMaskL1EMCALRelAll");
    trendMaskL0DCALRelAll->Write("trendMaskL0DCALRelAll");
    trendMaskL1DCALRelAll->Write("trendMaskL1DCALRelAll");
    trendwriter->mkdir("AbsFilter");
    trendwriter->cd("AbsFilter");
    trendMaskL0AllAbsFilter->Write("trendMaskL0AllAbsFilter");
    trendMaskL1AllAbsFilter->Write("trendMaskL1AllAbsFilter");
    trendMaskL0EMCALAbsFilter->Write("trendMaskL0EMCALAbsFilter");
    trendMaskL1EMCALAbsFilter->Write("trendMaskL1EMCALAbsFilter");
    trendMaskL0DCALAbsFilter->Write("trendMaskL0DCALAbsFilter");
    trendMaskL1DCALAbsFilter->Write("trendMaskL1DCALAbsFilter");
    trendwriter->mkdir("RelFilter");
    trendwriter->cd("RelFilter");
    trendMaskL0AllRelFilter->Write("trendMaskL0AllRelFilter");
    trendMaskL1AllRelFilter->Write("trendMaskL1AllRelFilter");
    trendMaskL0EMCALRelFilter->Write("trendMaskL0EMCALRelFilter");
    trendMaskL1EMCALRelFilter->Write("trendMaskL1EMCALRelFilter");
    trendMaskL0DCALRelFilter->Write("trendMaskL0DCALRelFilter");
    trendMaskL1DCALRelFilter->Write("trendMaskL1DCALRelFilter");
}