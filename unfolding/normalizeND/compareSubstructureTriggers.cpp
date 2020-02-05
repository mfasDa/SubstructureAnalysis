#include "../../meta/stl.C"
#include "../../meta/root.C"

void compareSubstructureTriggers(const char *observable = "Zg", const char *filename = "rawsoftdrop.root") {
    std::unique_ptr<TFile> reader(TFile::Open(filename, "READ"));
    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"},
                             filetype = {"pdf", "png", "eps"};

    std::map<std::string, std::string> obstitles = {{"Zg", "z_{g}"}, {"Rg", "R_{g}"}, {"Thetag", "#Theta_{g}"}, {"Nsd", "n_{SD}"}};
    auto obstitle = obstitles[observable];

    auto style = [](Color_t col, Style_t mrk) {
        return [col, mrk](auto obj) {
            obj->SetMarkerColor(col);
            obj->SetLineColor(col);
            obj->SetMarkerStyle(mrk);
        };
    };

    std::map<std::string, Color_t> colors = {{"INT7", kBlack}, {"EJ1", kRed}, {"EJ2", kBlue}};
    std::map<std::string, Style_t> markers = {{"INT7", 20}, {"EJ1", 24}, {"EJ2", 25}};

    for(auto R : ROOT::TSeqI(2, 7)){
        std::string rstring = Form("R%02d", R),
                    rtitle = Form("R = %.1f", double(R)/10.);
        reader->cd(rstring.data());

        std::map<std::string, TH2 *> rawcorrected;
        for(auto trg : triggers) rawcorrected[trg] = static_cast<TH2 *>(gDirectory->Get(Form("hZgVsPt%sCorrected", trg.data())));
        auto ptaxis = rawcorrected["INT7"]->GetYaxis();

        int iplot = 0, ipanel = 1;
        auto currentplot = new TCanvas(Form("ComparisonTriggers%s%d", rstring.data(), iplot), Form("Comparison triggers for %s (%d)", rtitle.data(), iplot), 1200, 800);
        currentplot->Divide(5,2);
        for(auto ipt : ROOT::TSeqI(0, ptaxis->GetNbins())) {
            if(ipanel == 11) {
                for(auto ft : filetype) currentplot->SaveAs(Form("%s.%s", currentplot->GetName(), ft.data()));
                iplot++;
                ipanel = 1;
                currentplot = new TCanvas(Form("ComparisonTriggers%s%d", rstring.data(), iplot), Form("Comparison triggers for %s (%d)", rtitle.data(), iplot), 1200, 800);
                currentplot->Divide(5,2);
            }
            currentplot->cd(ipanel);
            
            TLegend *leg(nullptr);
            if(ipanel == 1) {
                leg = new TLegend(0.15, 0.8, 0.89, 0.89);
                leg->SetBorderSize(0);
                leg->SetFillStyle(0);
                leg->SetTextFont(42);     
                leg->SetNColumns(3);               
            }

            double ptmin = ptaxis->GetBinLowEdge(ipt+1),
                   ptmax = ptaxis->GetBinUpEdge(ipt+1);
            auto ptlabel = new TPaveText(0.15, 0.15, 0.89, 0.22, "NDC");
            ptlabel->SetBorderSize(0);
            ptlabel->SetFillStyle(0);
            ptlabel->SetTextFont(42);                    
            ptlabel->AddText(Form("%.1f GeV/c < p_{t} < %.1f GeV/c", ptmin, ptmax));

            bool first = true;
            for(auto trg : triggers) {
                TH1 *hslice = rawcorrected[trg]->ProjectionX(Form("rawcorr%s_%s_%s_%d_%d", observable, trg.data(), rstring.data(), int(ptmin), int(ptmax)), ipt+1, ipt+1);
                hslice->SetDirectory(nullptr);
                hslice->SetStats(false);
                hslice->SetXTitle(obstitle.data());
                hslice->SetYTitle(Form("dN/d%s", obstitle.data()));
                style(colors[trg], markers[trg])(hslice);
                hslice->Draw(first ? "ep" : "epsame");
                first = false;
                if(leg) leg->AddEntry(hslice, trg.data(), "lep");
            }
            if(leg) leg->Draw();
            ptlabel->Draw();

            ipanel++;
        }
    }
}