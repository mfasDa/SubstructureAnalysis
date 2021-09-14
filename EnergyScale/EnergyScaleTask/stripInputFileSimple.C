#include "../../meta/stl.C"
#include "../../meta/root.C"

TList *buildReducedList(TList &input) {
	TList *result = new TList;
	result->SetName(input.GetName());
	std::vector<std::string> tags = {"hJetEnergyScale", "hJetResponseFine", "hJetSpectrumPartAll", "hPurityDet", "hJetfindingEfficiencyCore",
	};
	std::vector<std::string> histstokeep = {"fHistTrialsAfterSel",  "fHistEventsAfterSel",  
					"fHistXsectionAfterSel", "fHistTrials", "fHistEvents", 
					"fHistXsection", "fHistPtHard", "fHistPtHardCorr", 
					"fHistPtHardCorrGlobal", "fHistPtHardBinCorr", "fHistZVertex", 
					"fHistCentrality", "fHistEventPlane", "fCutStats", 
					"fCutStatsAfterTrigger", "fCutStatsAfterMultSelection", "fNormalisationHist", 
					"Vtz_raw", "DeltaVtz_raw", "Centrality_raw", "EstimCorrelation_raw", 
					"MultCentCorrelation_raw", "Vtz_selected", "DeltaVtz_selected", 
					"Centrality_selected", "EstimCorrelation_selected", 
					"MultCentCorrelation_selected", "fHistTriggerClasses", 
					"fHistEventCount", "hEventCounter"};
	for(auto hist : input) {
		auto istokeep = std::find(histstokeep.begin(), histstokeep.end(), hist->GetName()) != histstokeep.end();
		if(istokeep) {
			result->Add(hist);
		} else {
			std::string_view histname(hist->GetName());
			for(auto tag : tags) {
				if(histname.find(tag) != std::string::npos) {
					result->Add(hist);
					break;
				}
			}
		}
	}
	return result;
}

void stripInputFileSimple(const char *fname) {
	std::string_view indfiledecoder(fname);
	auto filebase = indfiledecoder.substr(0, indfiledecoder.find("."));
	std::string outfilename(filebase);
	outfilename += "_enscale.root";
	std::unique_ptr<TFile> reader(TFile::Open(fname, "READ")),
	                       writer(TFile::Open(outfilename.data(), "RECREATE"));

	for(auto input : TRangeDynCast<TKey>(reader->GetListOfKeys())) {
		std::string_view keyname(input->GetName());
		if(keyname.find("EnergyScaleResults") == std::string::npos) {
			continue;
		}
		std::cout << "Processing " << keyname << std::endl;
		reader->cd(keyname.data());
		auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();

		writer->mkdir(keyname.data());
		writer->cd(keyname.data());
		auto reducedlist = buildReducedList(*histlist);
		reducedlist->Write(reducedlist->GetName(), TObject::kSingleKey);
	}
}