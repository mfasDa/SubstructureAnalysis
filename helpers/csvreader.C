#include "../meta/stl.C"
class Table {
public:

	struct Row {
		std::map<std::string, std::string> entry;

		Row() = default;
		~Row() = default;
	};

	Table() = default;
	~Table() = default;

	Row *getRow(std::string key, std::string value) {
		Row *result = nullptr;
		auto found = std::find_if(mRows.begin(), mRows.end(), [key, value](const Row &row){
			auto test = row.entry.find(key);
			return test != row.entry.end() && test->second == value;
		});
		if(found != mRows.end()) {
			result = &(*found);
		}
		return result;
	};

	void insertRow(const std::map<std::string, std::string> &row) {
		mRows.push_back({row});
	}

	std::vector<Row> &getRows() { return mRows; }
	const std::vector<Row> &getRows() const { return mRows; }

	void importRows(const Table &other){
		for(const auto &row : other.getRows()) { insertRow(row.entry); }		
	}

private:
	std::vector<Row> mRows;
}; 

Table decodeCSV(const std::string filename) {
	std::ifstream in(filename);
	std::string tmp;
	std::vector<std::string> colnames;
	bool first = true;
	Table result;
	while(std::getline(in, tmp)) {
		std::stringstream decoder(tmp);
		std::string key;
		if(first) {
			// Decode column names
			while(std::getline(decoder, key, ',')) {
				colnames.emplace_back(key);
			}
			first = false;
		} else {
			std::map<std::string, std::string> row;
			int current = 0;
			while(std::getline(decoder, key, ',')) {
				row[colnames[current]] = key;
				current++;
			}
			result.insertRow(row);
		}
	}
	return result;
}