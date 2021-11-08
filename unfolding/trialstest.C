#include "../meta/stl.C"
#include "../meta/root.C"

class Trials {
public:
    Trials(TH1 *hist) { fHistogram = hist; };
    ~Trials() { delete fHistogram; }

    double getMaxTrials() const {
        auto bins = getBins();
        return *std::max_element(bins.begin(), bins.end());
    }

    double getAverageTrials() const {
        auto bins = getBins();
        return TMath::Mean(bins.begin(), bins.end());
    }

    double getTrialsFit() const {
        TF1 model("meanntrials", "pol0", 0., 100.);
        fHistogram->Fit(&model, "N", "", fHistogram->GetXaxis()->GetBinLowEdge(2), fHistogram->GetXaxis()->GetBinUpEdge(fHistogram->GetXaxis()->GetNbins()+1));
        return model.GetParameter(0);
    }

private:
    std::vector<double> getBins() const {
        std::vector<double> result;
        for(int ib = 0; ib < fHistogram->GetXaxis()->GetNbins(); ib++) {
            auto entries  = fHistogram->GetBinContent(ib+1);
            if(TMath::Abs(entries) > DBL_EPSILON) {
                // filter 0
                result.emplace_back(entries);
            }
        }
        return result;
    }

    TH1 *fHistogram;
};

Trials getTrials(TFile &reader, int R, const std::string_view sysvar) {
    std::stringstream dirnamebuilder;
    dirnamebuilder << "EnergyScaleResults_FullJet_R" << std::setw(2) << std::setfill('0') << R << "_INT7";
    if(sysvar.length()) dirnamebuilder << "_" << sysvar;
    reader.cd(dirnamebuilder.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto histtrials = static_cast<TH2 *>(histlist->FindObject("fHistTrials")); 
    histtrials->SetDirectory(nullptr);
    return Trials(histtrials);
}


void trialstest(const std::string_view mcfile, const std::string_view sysvar = "tc200"){
	std::unique_ptr<TFile> mcreader(TFile::Open(mcfile.data(), "READ"));
	for(auto R : ROOT::TSeqI(2, 7)) {
        auto trials = getTrials(*mcreader, R, sysvar);
        double radius = double(R)/10.;
        std::cout << "R="<< radius << ":"<<std::endl;
        std::cout << "Max:            " << trials.getMaxTrials() << std::endl;
        std::cout << "Average:        " << trials.getAverageTrials() << std::endl;
        std::cout << "Fit:            " << trials.getTrialsFit() << std::endl;
	}
}