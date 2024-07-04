#include "../meta/stl.C"
#include "../meta/root.C"

std::set<std::string> getOutlierCuts(TFile &reader) {
    std::set<std::string> result;
    for(auto dirname : TRangeDynCast<TKey>(reader.GetListOfKeys())){
        std::string keyname(dirname->GetName());
        std::string outliertag = keyname.substr(keyname.find("_")+1);
        if(result.find(outliertag) == result.end()) result.insert(outliertag);

    }
    return result;
}

std::set<std::string> matchDirectories(TFile &reader, const std::string_view outliertag) {
    std::set<std::string> result;
    for(auto dirname : TRangeDynCast<TKey>(reader.GetListOfKeys())){
        std::string keyname(dirname->GetName());
        if(keyname.find(outliertag) != std::string::npos) result.insert(keyname);
    }
    return result;
}

int getR(const std::string_view keyname) {
    int rpos = keyname.find_last_of("R");
    auto rstring = keyname.substr(rpos+1, 2);
    return atoi(rstring.data());
}

void extractPartLevelSpectrum(const char *filename = "AnalysisResults.root", int outliercut = -1) {
    std::map<int, TH1 *> spectra;
    {
        std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
        std::string outliertag = "";
        if(outliercut == -1) {
            outliertag = "nooutlier";
        } else {
            outliertag = Form("outlier%02d", outliercut*10);
        }
        auto outliertags = getOutlierCuts(*reader);
        if(outliertags.find(outliertag) == outliertags.end()) {
            std::cout << "No data found for outliercut " << outliertag << std::endl;
            return;
        }
        for(auto rdata : matchDirectories(*reader, outliertag)) {
            auto rval = getR(rdata);
            std::cout << "Reading R = " << float(rval) / 10. << std::endl; 
            auto histos = reader->Get<TList>(rdata.data());
            auto spectrum = static_cast<TH1 *>(histos->FindObject("hJetPt"));
            spectrum->SetDirectory(nullptr);
            spectra[rval] = spectrum;
        }
    }
    std::unique_ptr<TFile> writer(TFile::Open("PartLevelSpectra.root", "RECREATE"));
    writer->cd();
    for(auto &[rval, spectrum] : spectra) {
        spectrum->SetNameTitle(Form("hJetSpectrumR%02d", rval), Form("Jet spectrum for R=%.1f", double()/10.));
        spectrum->Write();
    }
}