#include "../meta/stl.C"
#include "../meta/root.C"
#include "csvreader.C"

class TriggerRawDB {
public:
	TriggerRawDB() = default;
	~TriggerRawDB() = default;

	bool hasRun(int runnumber) { return mData.getRow("run", runnumber) != nullptr; }
	int getTriggersForRun(int runnumber, const char *trigger) {
		auto tmp = mData.getRow("run", runnumber);
		if(tmp) {
			auto found = tmp->entry.find(trigger);
			if(found != tmp->entry.end()) return found->second;
		}
		return 0;
	}

	void addFile(std::string filename){
		auto tmp = decodeCSV(filename);
		if(mData.getRows().size()) mData.importRows(tmp);
		else mData=tmp;
	}

	void buildForYear(int year) {
		int yeartag = year % 100;
		std::string csvdir = Form("%s/RawEventCounts", gSystem->Getenv("SUBSTRUCTURE_ROOT"));
		std::vector<std::string> filenames;
		auto filestring = gSystem->GetFromPipe(Form("ls -1 %s | grep LHC%s | grep -v bad"));
		std::unique_ptr<TObjArray> tokened(filestring.Tokenize("\n"));
		for(auto f : TRangeDynCast<TObjString>(tokened.get())) {
			filenames.emplace_back(f->String().Data());
		}
		std::sort(filenames.begin(), filenames.end(), std::less<std::string>());
		for(auto f : filenames) {
			std::string path = Form("%s/%s", csvdir.data(), f.data());
			addFile(path);
		}
	}

private:
	Table mData;
};