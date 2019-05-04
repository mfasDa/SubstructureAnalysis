#ifndef __RESPONSEREADER_H__
#define __RESPONSEREADER_H__

#include "../meta/root.C"
#include "../meta/stl.C"

class ResponseData {
public:
    ResponseData() = default;
    ~ResponseData() = default;

    void AddResponseMatrix(const std::string_view name, TH2 *matrix) {
        mData[name.data()] = matrix;        
    }

    TH2 *GetResponseMatrix(const std::string_view name) const {
        TH2 *result(nullptr);
        auto found = mData.find(name.data());
        if(found != mData.end()){
            result = found->second;
        }
        return result;
    }

    TH2 *GetResponseMatrixFine() const { return GetResponseMatrix("fine"); }
    TH2 *GetResponseMatrixTruncated() const { return GetResponseMatrix("truncated"); }
    
private:
    std::map<std::string, TH2 *>            mData;
};

class ResponseReader {
public:
    ResponseReader() = default;
    ResponseReader(const std::string_view inputfile) : mData() { ReadFile(inputfile); }
    ~ResponseReader() = default;

    TH2 *GetResponseMatrix(double r, const std::string_view type) const {
        TH2 *result(nullptr);
        auto rdata = mData.find(r);
        if(rdata != mData.end()) {
            result = rdata->second->GetResponseMatrix(type);
        }
        return result;
    }

    TH2 *GetResponseMatrixFine(double r) const { return GetResponseMatrix(r, "fine"); }
    TH2 *GetResponseMatrixTruncated(double r) const { return GetResponseMatrix(r, "truncated"); }

    std::vector<double> getJetRadii() const {
        std::vector<double> result;
        for(const auto &en : mData) {
            result.emplace_back(en.first);
        }
        return result;
    }

protected:
    void ReadFile(const std::string_view filename) {
        std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
        for(auto rad : *reader->GetListOfKeys()) {
            const std::string rstring(rad->GetName());
            double rval = double(std::stoi(rstring.substr(1)))/10.;
            reader->cd(rstring.data());
            gDirectory->cd("response");
            TH2 *responsefine = static_cast<TH2 *>(gDirectory->Get(Form("Rawresponse_%s_fine", rstring.data()))),
                *responsetruncated = static_cast<TH2 *>(gDirectory->Get(Form("Rawresponse_%s_fine_rebinned_standard", rstring.data())));
            responsefine->SetDirectory(nullptr);
            responsetruncated->SetDirectory(nullptr);
            ResponseData *entry = nullptr;
            auto data = mData.find(rval);
            if(data != mData.end()) {
                entry = data->second.get();
            } else {
                entry = new ResponseData;
                mData[rval] = std::shared_ptr<ResponseData>(entry);
            }
            entry->AddResponseMatrix("fine", responsefine);
            entry->AddResponseMatrix("truncated", responsetruncated);
        }
    }

private:
    std::map<double, std::shared_ptr<ResponseData>>        mData;
};

#endif