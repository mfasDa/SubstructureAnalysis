#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../meta/stl.C"

void makeComparisonIterations(const char *inputfile = "UnfoldedSD.root", const char *observable = "all"){
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
            std::map<int, TH2 *> iterations;
            std::vector<std::pair<double, double>> ptbins;
            double obsmin, obsmax;
            for(auto iter : ROOT::TSeqI(1, 31)) {
                rdir->cd(Form("Iter%d", iter));
                auto unfoldedspectrum = static_cast<TH2 *>(gDirectory->Get(Form("unfoldedIter%d_%s_%s", iter, obsname.data(), rstring.data())));
                unfoldedspectrum->SetDirectory(nullptr);
                if(!ptbins.size()) {
                    for(auto ib : ROOT::TSeqI(0, unfoldedspectrum->GetYaxis()->GetNbins())) {
                        ptbins.push_back({unfoldedspectrum->GetYaxis()->GetBinLowEdge(ib+1), unfoldedspectrum->GetYaxis()->GetBinUpEdge(ib+1)});
                    }
                    obsmin = unfoldedspectrum->GetXaxis()->GetBinLowEdge(1);
                    obsmax = unfoldedspectrum->GetXaxis()->GetBinUpEdge(unfoldedspectrum->GetXaxis()->GetNbins());
                }
                iterations[iter] = unfoldedspectrum;
            }


            int plotcounter = 0, currentcol = 0; 
            ROOT6tools::TSavableCanvas *currentplot(nullptr);
            for(auto iptb : ROOT::TSeqI(0, ptbins.size())) {
                if((currentcol % 5) == 0) {
                    if(currentplot) currentplot->SaveCanvas(currentplot->GetName());
                    currentcol = 0;
                    currentplot = new ROOT6tools::TSavableCanvas(Form("CompUnfoldInterations_%s_%s_sub%d", obsname.data(), rstring.data(), plotcounter), Form("Comparison fold/raw for %s for %s (%d)", obstitle.data(), rtitle.data(), plotcounter), 1200, 600);
                    currentplot->Divide(5,2);
                    plotcounter++;
                } 

                auto ptmin = ptbins[iptb].first, ptmax = ptbins[iptb].second;
                std::cout << "Doing " << ptmin << " GeV/c < p_{t} < " << ptmax << " GeV/c" << std::endl;

                TLegend *leg = nullptr;
                if(!currentcol) {
                    printf("Creating new legend\n");
                    leg = new ROOT6tools::TDefaultLegend(0.5, 0.25, 0.89, 0.89);
                    leg->SetTextSize(0.045);
                }

                currentplot->cd(currentcol+6);
                gPad->SetLeftMargin(0.15);
                gPad->SetRightMargin(0.05);
                auto ratioframe = new ROOT6tools::TAxisFrame(Form("ratioframe_%s_%s_ipt%d", obsname.data(), rstring.data(), iptb), obstitle.data(), "Ratio to previous", obsmin, obsmax, 0.7, 1.3);
                ratioframe->GetXaxis()->SetTitleSize(0.045);
                ratioframe->GetYaxis()->SetTitleSize(0.045);
                ratioframe->Draw("axis");

                TH1 *cache (nullptr);
                for(auto iter : ROOT::TSeqI(1, 10)) {
                    auto sliceiter = iterations[iter]->ProjectionX(Form("refold%s_%s_%d_%d_iter%d", obsname.data(), rstring.data(), int(ptmin), int(ptmax), iter), iptb+1, iptb+1);
                    sliceiter->SetDirectory(nullptr);
                    sliceiter->SetTitle("");
                    sliceiter->SetStats(false);
                    sliceiter->SetXTitle(obstitle.data());
                    sliceiter->SetYTitle(Form("dN/dp_{t}d%s ((GeV/c)^{-1})", obstitle.data()));
                    sliceiter->GetXaxis()->SetTitleSize(0.045);
                    sliceiter->GetYaxis()->SetTitleSize(0.045);
                    style(colors[iter-1], markers[iter-1])(sliceiter);
                    currentplot->cd(currentcol+1);
                    if(iter == 1) {
                        gPad->SetLeftMargin(0.15);
                        gPad->SetRightMargin(0.05);
                    }
                    sliceiter->Draw(iter == 1 ? "ep" : "epsame");
                    if(leg) leg->AddEntry(sliceiter, Form("Iteration %d", iter));

                    if(iter !=1) {
                        auto ratio = static_cast<TH1 *>(sliceiter->Clone(Form("ratio_iter%d_raw_%s_%s_%d_%d", iter, obsname.data(), rstring.data(), int(ptmin), int(ptmax))));
                        ratio->SetDirectory(nullptr);
                        ratio->Divide(cache);
                        currentplot->cd(currentcol+6);
                        ratio->Draw("epsame");
                    }
                    cache = sliceiter;
                }
                currentplot->cd(currentcol+1);
                (new ROOT6tools::TNDCLabel(0.2, 0.9, 0.94, 0.98, Form("%.1f GeV/c < p_{t,j,det} < %.1f GeV/c", ptmin, ptmax)))->Draw();
                if(leg) {
                    leg->Draw();
                }
                currentcol++;
            }
            if(currentplot) currentplot->SaveCanvas(currentplot->GetName());
        }
    }
}