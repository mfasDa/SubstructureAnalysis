#ifndef JETSPECTRUMCONTAINER_C
#define JETSPECTRUMCONTAINER_C
#include "../meta/stl.C"
#include "../meta/root.C"

class JetSpectrumData {
public:
    JetSpectrumData() = default;
    ~JetSpectrumData() = default;

    void SetJetSpectrum(const std::string_view type, TH1 *spectrum) { mData[type.data()] = spectrum; }
    TH1 *GetJetSpectrum(const std::string_view type) {
        TH1 *result = nullptr;
        auto found = mData.find(type.data());
        if(found != mData.end()) {
            result = found->second;
        }
        return result;
    }

private:
    std::map<std::string, TH1 *>                            mData;
};

class JetSpectrumContainer {
public:
    JetSpectrumContainer() = default;
    ~JetSpectrumContainer() = default;

    void InsertJetSpectrum(double radius, const std::string_view type, TH1 *spectrum) {
        JetSpectrumData *mydata(nullptr);
        auto found = mData.find(radius);
        if(found != mData.end()) {
            mydata = found->second.get();
        } else {
            mydata = new JetSpectrumData;
            mData[radius] = std::shared_ptr<JetSpectrumData>(mydata);
        }
        mydata->SetJetSpectrum(type, spectrum);
    }

    TH1 *GetJetSpectrum(double radius, const std::string_view type) {
        TH1 *result(nullptr);
        auto found = mData.find(radius);
        if(found != mData.end()){
           result = found->second->GetJetSpectrum(type); 
        }
        return result;
    }

    std::vector<double> GetJetRadii() const {
        std::vector<double> jetradii;
        for(auto &k : mData) {
            jetradii.push_back(k.first);
        }
        return jetradii;
    }

private:
    std::map<double, std::shared_ptr<JetSpectrumData>>       mData;
};

#endif