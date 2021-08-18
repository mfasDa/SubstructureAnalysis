#include "../meta/stl.C"
#include "../meta/root.C"
#include "csvreader.C"

class TriggerRawDB {
public:
	TriggerRawDB() = default;
	~TriggerRawDB() = default;

	bool hasRun(int runnumber) { return mData.getRow("run", Form("%d", runnumber)) != nullptr; }
	int getTriggersForRun(int runnumber, const char *trigger) {
		auto tmp = mData.getRow("run", Form("%d", runnumber));
		if(tmp) {
			auto found = tmp->entry.find(trigger);
			if(found != tmp->entry.end()) return std::stoi(found->second);
		}
		return 0;
	}
	std::string getPeriod(int runnumber){
		auto tmp = mData.getRow("run", Form("%d", runnumber));
		if(tmp) {
			auto found = tmp->entry.find("period");
			if(found != tmp->entry.end()) return found->second;
		}
		return "";
	}

	void addFile(std::string filename, std::string period){
		auto tmp = decodeCSV(filename);
		for(auto &row : tmp.getRows()) {
			row.entry[std::string("period")] = period;
		}
		if(mData.getRows().size()) mData.importRows(tmp);
		else mData=tmp;
	}

	void buildForYear(int year) {
		int yeartag = year % 100;
		std::string csvdir = Form("%s/RawEventCounts", gSystem->Getenv("SUBSTRUCTURE_ROOT"));
		std::vector<std::string> filenames;
		auto filestring = gSystem->GetFromPipe(Form("ls -1 %s | grep LHC%d | grep -v bad", csvdir.data(), yeartag));
		std::unique_ptr<TObjArray> tokened(filestring.Tokenize("\n"));
		for(auto f : TRangeDynCast<TObjString>(tokened.get())) {
			filenames.emplace_back(f->String().Data());
		}
		std::sort(filenames.begin(), filenames.end(), std::less<std::string>());
		for(auto f : filenames) {
			std::string path = Form("%s/%s", csvdir.data(), f.data());
			std::string period = f.substr(0, f.find(".csv"));
			std::cout << "Reading " << path << std::endl;
			addFile(path, period);
		}
	}

private:
	Table mData;
};