#include "../helpers/graphics.C"

struct energyscaleresults {
    TH1 *hmean;
    TH1 *hsigma;
};

std::map<double, energyscaleresults> getEnergyScaleResults(const std::string_view filename) {
    std::map<double, energyscaleresults> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(auto r = 0.2; r <= 0.6; r += 0.1) {
        std::string rstring = Form("R%02d", int(r*10.));
        result[r] = {static_cast<TH1 *>(reader->Get(Form("EnergyScale_%s_mean", rstring.data()))), static_cast<TH1 *>(reader->Get(Form("EnergyScale_%s_width", rstring.data())))};
    }
    return result;
}

void comparisonEnergyScale() {
    auto datawith = getEnergyScaleResults("withTRD/mc/EnergyScaleResults_notc.root"),
         datawithout = getEnergyScaleResults("withoutTRD/mc/EnergyScaleResults_notc.root");

    auto plot = new ROOT6tools::TSavableCanvas("energyScaleCompTRD", "Energy scale comparison with/without TRD", 300 * datawith.size(), 700);
    plot->Divide(datawith.size(), 2);

    Style withstyle{kRed, 24}, withoutstyle{kBlue, 25};

    int icol = 0;
    for(auto r = 0.2; r <= 0.6; r += 0.1) {
        plot->cd(icol + 1);
        std::string rstring = Form("R%02d", int(r*10.));
        (new ROOT6tools::TAxisFrame(Form("meanframe_%s", rstring.data()), "p_{t,part}", "|(p_{t,det} - p_{t,part})/p_{t,part}|", 0., 400., -1., 1.))->Draw("axis");
        (new ROOT6tools::TNDCLabel(0.15, 0.15, 0.4, 0.22, Form("R=%.1f", r)))->Draw();
        TLegend *leg(nullptr);
        if(!icol) {
            leg = new ROOT6tools::TDefaultLegend(0.65, 0.75, 0.89, 0.89);
            leg->Draw();
        }
        auto enwith = datawith.find(r)->second,
             enwithout = datawithout.find(r)->second;
        withstyle.SetStyle<TH1>(*enwith.hmean);
        withoutstyle.SetStyle<TH1>(*enwithout.hmean);
        enwith.hmean->Draw("epsame");
        enwithout.hmean->Draw("epsame");
        if(leg){
            leg->AddEntry(enwith.hsigma, "with TRD", "lep");
            leg->AddEntry(enwithout.hsigma, "without TRD", "lep");
        }

        plot->cd(icol + 1 + datawith.size());
        (new ROOT6tools::TAxisFrame(Form("widthframe_%s", rstring.data()), "p_{t,part}", "#sigma((p_{t,det} - p_{t,part})/p_{t,part})", 0., 400., 0., 0.5))->Draw("axis");
        withstyle.SetStyle<TH1>(*enwith.hsigma);
        withoutstyle.SetStyle<TH1>(*enwithout.hsigma);
        enwith.hsigma->Draw("epsame");
        enwithout.hsigma->Draw("epsame");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}