#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"

void extractFastORECorr(const std::string_view inputfile = "AnalysisResults.root", const std::string_view trigger = "INT7"){
    std::array<TH2 *, 20> projectionSM;
    {
        std::vector<std::pair<double, double>> limits = {{0, 287}, {288, 575}, {576, 863}, {864, 1151}, {1152, 1439}, 
                                                         {1440, 1727}, {1728, 2015}, {2016, 2303}, {2304, 2591}, {2592, 2879},
                                                         {2880, 2975}, {2976, 3071},
                                                         {3072, 3263}, {3456, 3647}, {3648, 3839}, {4032, 4223}, 
                                                         {4224, 4415}, {4608, 4799}, 
                                                         {4800, 4895}, {4896, 4991}                                                         
                                                         };
        std::unique_ptr<TFile> reader(TFile::Open(inputfile.data(), "READ"));
        reader->cd(Form("fastorMonitor%s", trigger.data()));
        auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
        auto hsparse = static_cast<THnSparse *>(histlist->FindObject("hFastOrEnergyOfflineOnline"));
        for(auto ism : ROOT::TSeqI(0, 20)){
            auto fastorRange = limits[ism];
            hsparse->GetAxis(0)->SetRange(hsparse->GetAxis(0)->FindBin(fastorRange.first), hsparse->GetAxis(0)->FindBin(fastorRange.second));   
            auto projected = hsparse->Projection(2,1);
            projected->SetDirectory(nullptr);
            projected->SetName(Form("ADCcorrSM%d", ism));
            projected->SetTitle(Form("Supermodule %d", ism));
            projected->SetStats(false);
            projected->SetXTitle("FEE energy (GeV)");
            projected->SetXTitle("FastOR energy (GeV)");
            projectionSM[ism] = projected;
        }
    }

    auto plot = new ROOT6tools::TSavableCanvas("CorrelationEnergySM", "Correlation energy supermodule", 1200, 1000);
    plot->Divide(5,4);
    int ipad = 1;
    for(auto ism : ROOT::TSeqI(0, 20)) {
        plot->cd(ipad++);
        gPad->SetLogz();
        projectionSM[ism]->Draw("colz");
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}