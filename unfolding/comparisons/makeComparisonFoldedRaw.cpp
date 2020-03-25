#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../meta/stl.C"

void makeComparisonFoldedRaw(const char *inputfile = "UnfoldedSD.root", const char *observable = "all"){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ"));

    auto style = [](Color_t col, Style_t marker) {
        return [col, marker] (auto obj) {
            obj->SetMarkerColor(col);
            obj->SetMarkerStyle(marker);
            obj->SetLineColor(col);
        };
    };

    std::vector<std::string> observablesAll =  {"Zg", "Rg", "Nsd", "Thetag"}, observablesSelected;
    if(std::string_view(observable) == std::string_view("all")) observablesSelected = observablesAll;
    else observablesSelected.push_back(observable);
    std::map<std::string, std::string> obstitles = {{"Zg", "z_{g}"}, {"Rg", "r_{g}"}, {"Nsd", "n_{SD}"}, {"Thetag", "#Theta_{g}"}};
    std::vector<Color_t> colors = {kRed, kBlue, kGreen, kOrange, kCyan, kMagenta, kGray, kTeal, kViolet, kAzure};
    std::vector<Style_t> markers = {24, 25, 26, 27, 28, 29, 30, 31, 32, 33};


    for(auto obs : TRangeDynCast<TKey>(gDirectory->GetListOfKeys())) {
        std::string_view obsname(obs->GetName());
        if(std::find_if(observablesSelected.begin(), observablesSelected.end(), [&obsname](const std::string &test) {return test == std::string(obsname); }) == observablesSelected.end()) continue;
        auto obstitle = obstitles[obsname.data()];
        std::cout << "Processing observable " << obsname << std::endl;

        reader->cd(obs->GetName());
        auto obsbasedir = gDirectory;

        for(auto R : ROOT::TSeqI(2, 7)) {
            std::string rstring = Form("R%02d", R),
                        rtitle = Form("R = %.1f", double(R)/10.);
            obsbasedir->cd(rstring.data());
            auto rdir = gDirectory;
            rdir->cd("rawlevel");
            auto rawspectrum = static_cast<TH2 *>(gDirectory->Get(Form("rawlevel%s_%s_combined", obsname.data(), rstring.data())));
            rawspectrum->SetDirectory(nullptr);
            std::map<int, TH2 *> iterations;
            for(auto iter : ROOT::TSeqI(1, 31)) {
               rdir->cd(Form("Iter%d", iter));
               auto refoldedspectrum = static_cast<TH2 *>(gDirectory->Get(Form("refoldedIter%d_%s_%s", iter, obsname.data(), rstring.data())));
               refoldedspectrum->SetDirectory(nullptr);
               iterations[iter] = refoldedspectrum;
            }

            std::vector<std::pair<double, double>> ptbins;
            for(auto ib : ROOT::TSeqI(0, rawspectrum->GetYaxis()->GetNbins())) {
                ptbins.push_back({rawspectrum->GetYaxis()->GetBinLowEdge(ib+1), rawspectrum->GetYaxis()->GetBinUpEdge(ib+1)});
            }

            int plotcounter = 0, currentcol = 0; 
            ROOT6tools::TSavableCanvas *currentplot(nullptr);
            for(auto iptb : ROOT::TSeqI(0, ptbins.size())) {
                if((currentcol % 5) == 0) {
                    if(currentplot) currentplot->SaveCanvas(currentplot->GetName());
                    currentcol = 0;
                    currentplot = new ROOT6tools::TSavableCanvas(Form("CompFoldRaw_%s_%s_sub%d", obsname.data(), rstring.data(), plotcounter), Form("Comparison fold/raw for %s for %s (%d)", obstitle.data(), rtitle.data(), plotcounter), 1200, 600);
                    currentplot->Divide(5,2);
                    plotcounter++;
                } 

                auto ptmin = ptbins[iptb].first, ptmax = ptbins[iptb].second;
                std::cout << "Doing " << ptmin << " GeV/c < p_{t} < " << ptmax << " GeV/c" << std::endl;
                auto sliceraw = rawspectrum->ProjectionX(Form("raw%s_%s_%d_%d", obsname.data(), rstring.data(), int(ptmin), int(ptmax)), iptb+1, iptb+1);
                sliceraw->SetDirectory(nullptr);
                sliceraw->SetTitle("");
                sliceraw->SetStats(false);
                sliceraw->SetXTitle(obstitle.data());
                sliceraw->SetYTitle(Form("1/N_{ev} dN/p_{t}d%s", obstitle.data()));
                sliceraw->GetXaxis()->SetTitleSize(0.045);
                sliceraw->GetYaxis()->SetTitleSize(0.045);
                style(kBlack, 20)(sliceraw);

                auto obsmin = sliceraw->GetXaxis()->GetBinLowEdge(1), obsmax = sliceraw->GetXaxis()->GetBinUpEdge(sliceraw->GetXaxis()->GetNbins());

                currentplot->cd(currentcol+1);
                gPad->SetLeftMargin(0.15);
                gPad->SetRightMargin(0.05);
                sliceraw->Draw("pe");
                TLegend *leg = nullptr;
                if(!currentcol) {
                    leg = new ROOT6tools::TDefaultLegend(0.65, 0.45, 0.89, 0.89);
                    leg->SetTextSize(0.045);
                    leg->Draw();
                }
                (new ROOT6tools::TNDCLabel(0.15, 0.9, 0.89, 0.98, Form("%.1f GeV/c < p_{t,j,det} < %.1f GeV/c", ptmin, ptmax)))->Draw();

                currentplot->cd(currentcol+6);
                gPad->SetLeftMargin(0.15);
                gPad->SetRightMargin(0.05);
                auto ratioaxis = new ROOT6tools::TAxisFrame(Form("ratioframe_%s_%s_ipt%d", obsname.data(), rstring.data(), iptb), obstitle.data(), "Refolded / raw", obsmin, obsmax, 0., 2.);
                ratioaxis->Draw("axis");

                for(auto iter : ROOT::TSeqI(1, 10)) {
                    auto sliceiter = iterations[iter]->ProjectionX(Form("refold%s_%s_%d_%d_iter%d", obsname.data(), rstring.data(), int(ptmin), int(ptmax), iter), iptb+1, iptb+1);
                    sliceiter->SetDirectory(nullptr);
                    style(colors[iter-1], markers[iter-1])(sliceiter);
                    currentplot->cd(currentcol+1);
                    sliceiter->Draw("epsame");
                    if(leg) leg->AddEntry(sliceiter, Form("Iteration %d", iter), "lep");

                    auto ratio = static_cast<TH1 *>(sliceiter->Clone(Form("ratio_iter%d_raw_%s_%s_%d_%d", iter, obsname.data(), rstring.data(), int(ptmin), int(ptmax))));
                    ratio->SetDirectory(nullptr);
                    ratio->Divide(sliceraw);
                    currentplot->cd(currentcol+6);
                    ratio->Draw("epsame");
                }
                currentcol++;
            }
            if(currentplot) currentplot->SaveCanvas(currentplot->GetName());
        }
    }
}