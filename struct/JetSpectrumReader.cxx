#ifndef JETSPECTRUMREADER_C
#define JETSPECTRUMREADER_C
#include "../meta/stl.C"
#include "../meta/root.C"
#include "../helpers/string.C"
#include "JetSpectrumContainer.cxx"

class JetSpectrumReader {
public:
    JetSpectrumReader() = default;
    JetSpectrumReader(const std::string_view filename, std::vector<std::string> types) : mData(), mTypes(types) { ReadFile(filename); } 
    ~JetSpectrumReader() = default;

    JetSpectrumContainer GetJetSpectra() const { return mData; }
    const JetSpectrumContainer &GetDataRef() const {return mData; }
    TH1 *GetJetSpectrum(double radius, const std::string_view type) { return mData.GetJetSpectrum(radius, type); }

protected:
    bool IsRawType(const std::string_view type) {
        if(contains(type, "raw") || contains(type, "Raw")) return true;
        return false;
    }

    std::vector<std::string> getKeylist(TDirectory *dir) {
        std::vector<std::string> keys;
        for(auto k : *dir->GetListOfKeys()){
            keys.push_back(k->GetName());
        }
        return keys;
    }

    std::string findKey(std::vector<std::string> keys, std::vector<std::string> tokens) {
        std::string result;
        for(const auto k : keys){
            bool found(true);
            for(const auto &t : tokens){
                if(!contains(k, t)){
                    found = false;
                    break;
                }
            }
            if(found) {
                result = k;
                break;
            }
        }
        return result;
    }

    void ReadFile(const std::string_view filename) {
        std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
        for(auto en : *reader->GetListOfKeys()) {
            std::string rstring(en->GetName());
            double radius = double(std::stoi(rstring.substr(1))/10.);
            reader->cd(rstring.data());
            auto basedir = gDirectory;
            for(const auto &type : mTypes) {
                basedir->cd();
                if(contains(type, "reg")) {
                    std::string regstring = type.substr(type.find("reg"));
                    basedir->cd(regstring.data());
                    auto hist = static_cast<TH1 *>(gDirectory->Get(type.data()));
                    hist->SetDirectory(nullptr);
                    mData.InsertJetSpectrum(radius, type, hist);
                } else if (IsRawType(type)) {
                    basedir->cd("rawlevel");
                    auto keys = getKeylist(gDirectory);
                    std::string tkey;
                    bool donorm(false), dowidth(false);
                    if(contains(type, "hraw")) {
                        tkey = findKey(keys, {"hraw"});
                        if(!contains(type, "nowidth")) dowidth = true;
                    } else if(contains(type, "RawJetSpectrum")) {
                        auto config = tokenize(type, '_');
                        auto trigger = config[1];
                        tkey = findKey(keys, {"RawJetSpectrum", trigger});
                        if(contains(type, "norm")) donorm = true;
                    }
                    if(tkey.length()){
                        auto hist = static_cast<TH1 *>(gDirectory->Get(tkey.data()));
                        hist->SetDirectory(nullptr);
                        if(dowidth) hist->Scale(1., "width");
                        if(donorm) {
                            auto normhist = static_cast<TH1 *>(gDirectory->Get("hnorm"));
                            hist->Scale(1./normhist->GetBinContent(1));
                        }
                        mData.InsertJetSpectrum(radius, type, hist);
                    }
                } else if(contains(type, "truefull")) {
                    basedir->cd("response");
                    auto keys = getKeylist(gDirectory);
                    auto tkey = findKey(keys, {"truefull"});
                    if(tkey.length()){
                        auto hist = static_cast<TH1 *>(gDirectory->Get(tkey.data()));
                        hist->SetDirectory(nullptr);
                        mData.InsertJetSpectrum(radius, type, hist);
                    }
                } else if(contains(type, "truetrunc")){
                    basedir->cd("response");
                    auto responsematrix = static_cast<TH2 *>(gDirectory->Get(Form("Rawresponse_%s_fine_rebinned_standard", rstring.data())));
                    auto hist = responsematrix->ProjectionY(Form("truetrunc_%s", rstring.data()));
                    hist->SetDirectory(nullptr);
                    mData.InsertJetSpectrum(radius, "truetrunc", hist);
                } else if(contains(type, "mcdet")){
                    basedir->cd("response");
                    auto responsematrix = static_cast<TH2 *>(gDirectory->Get(Form("Rawresponse_%s_fine", rstring.data())));
                    auto hist = responsematrix->ProjectionX(Form("mcdet_%s", rstring.data()));
                    hist->SetDirectory(nullptr);
                    mData.InsertJetSpectrum(radius, "mcdet", hist);
                } else if(contains(type, "closure")) {
                    basedir->cd("closuretest");
                    auto keys = getKeylist(gDirectory);
                    auto tkey = findKey(keys, {type});
                    if(tkey.length()){
                        auto hist = static_cast<TH1 *>(gDirectory->Get(tkey.data()));
                        hist->SetDirectory(nullptr);
                        mData.InsertJetSpectrum(radius, type, hist);
                    }
                }
            }
        }
    }

private:
    JetSpectrumContainer                mData;
    std::vector<std::string>            mTypes;
};

#endif // JETSPECTRUMREADER