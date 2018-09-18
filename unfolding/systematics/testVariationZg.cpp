#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"
#include "../../helpers/substructuretree.C"

struct ptbindata {
    double ptmin;
    double ptmax;
    TH1 *bindata;
};

std::vector<ptbindata> getCorrected(const std::string_view filename, const std::string_view varname, int iteration = 10) {
    std::vector<ptbindata> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    // get kinematic efficiencies:
    std::vector<ptbindata> efficiencies;
    for(auto k : TRangeDynCast<TKey>(reader->GetListOfKeys())){
        if(contains(k->GetName(), "efficiency")){
            double ptmin, ptmax;
            auto tokens = tokenize(k->GetName(), '_');
            ptmin = std::stoi(tokens[1]); ptmax = std::stoi(tokens[2]);
            efficiencies.push_back({ptmin, ptmax, k->ReadObject<TH1>()});
        }
    }
    auto efffinder = [&efficiencies] (double ptmin, double ptmax) -> TH1* {
        TH1 *result = nullptr;
        auto found = std::find_if(efficiencies.begin(), efficiencies.end(), [&ptmin, &ptmax] (const ptbindata &bd) { return TMath::Abs(bd.ptmin - ptmin) < DBL_EPSILON && TMath::Abs(bd.ptmax - ptmax) < DBL_EPSILON; });
        if(found != efficiencies.end()) result = found->bindata;
        return result;
    };
    reader->cd(Form("iteration%d", iteration));
    auto h2d = static_cast<TH2 *>(gDirectory->Get(Form("zg_unfolded_iter%d", iteration)));
    for(auto b : ROOT::TSeqI(0, h2d->GetYaxis()->GetNbins())){
        double ptmin = h2d->GetYaxis()->GetBinLowEdge(b+1), ptmax = h2d->GetYaxis()->GetBinUpEdge(b+1);
        auto projected = h2d->ProjectionX(Form("projectionIter%dzg_%s_pt%d_%d", iteration, varname.data(), int(ptmin), int(ptmax)), b+1, b+1);
        projected->SetDirectory(nullptr);
        auto eff = efffinder(ptmin, ptmax);
        if(eff) {
            projected->Divide(eff);
        } else {
            std::cout << "Efficiency not found for pt bin " << ptmin << " to  " << ptmax << std::endl;
        }
        // Correct for the bin width
        projected->Scale(1./projected->Integral());
        normalizeBinWidth(projected);
        result.push_back({ptmin, ptmax, projected});
    }
    return result;
}

TH1 *makeBarlow(const TH1 *variation, const TH1 *defaultdist){
    auto barlow = histcopy(defaultdist);
    barlow->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, defaultdist->GetXaxis()->GetNbins())) {
        auto diff = TMath::Abs(defaultdist->GetBinContent(b+1) - variation->GetBinContent(b+1)),
             sigma = TMath::Sqrt(TMath::Abs(TMath::Power(defaultdist->GetBinError(b+1), 2) - TMath::Power(variation->GetBinError(b+1), 2)));
        barlow->SetBinContent(b+1, diff/sigma);
        barlow->SetBinError(b+1, 0);
    }
    return barlow;
}

TH1 *makeRatio(const TH1 *variation, const TH1 *defaultdist){
    auto ratio = histcopy(variation);
    ratio->SetDirectory(nullptr);
    ratio->Divide(defaultdist);
    return ratio;
}

void testVariationZg(const std::string_view varname, const std::string_view vartitle, const std::string_view defaultfile, const std::string_view varfile){
    auto spectraDefault = getCorrected(defaultfile, "default"), spectraVariation = getCorrected(varfile, "variation");

    auto tag = getFileTag(defaultfile);
    auto jd = getJetType(tag);
    tag.erase(tag.find("_unfolded_zg"), strlen("_unfolded_zg"));
    auto compplot = new ROOT6tools::TSavableCanvas(Form("comp_%s_%s", varname.data(), tag.data()), Form("Comparison %s %s", varname.data(), tag.data()), 1200, 1000);
    compplot->DivideSquare(spectraDefault.size());
    auto ratioplot = new ROOT6tools::TSavableCanvas(Form("ratioVarDefault_%s_%s", varname.data(), tag.data()), Form("Ratio Variation/Default %s %s", varname.data(), tag.data()), 1200, 1000);
    ratioplot->DivideSquare(spectraDefault.size());
    auto barlowplot = new ROOT6tools::TSavableCanvas(Form("barlow_%s_%s", varname.data(), tag.data()), Form("Barlow criterion %s %s", varname.data(), tag.data()), 1200, 1000);
    barlowplot->DivideSquare(spectraDefault.size());

    auto binfinder = [](std::vector<ptbindata> data, double ptmin, double ptmax) -> TH1 *{
        auto found = std::find_if(data.begin(), data.end(), [&ptmin, &ptmax](const ptbindata &bind) { return TMath::Abs(bind.ptmin - ptmin) < DBL_EPSILON && TMath::Abs(bind.ptmax - ptmax) < DBL_EPSILON; });
        if(found != data.end()) return found->bindata;
        return nullptr;
    };

    Style defaultstyle{kRed, 24}, varstyle{kBlue, 25};

    int ipad = 1;
    std::vector<TH1 *> ratiohists, barlowhists;
    for(auto spec : spectraDefault) {
        auto label = new ROOT6tools::TNDCLabel(0.15, 0.9, 0.55, 0.97, Form("%.1f GeV/c < p_{t} < %.1f GeV/c", spec.ptmin, spec.ptmax));
        auto varspec = binfinder(spectraVariation, spec.ptmin, spec.ptmax);

        compplot->cd(ipad);
        (new ROOT6tools::TAxisFrame(Form("compframe%d", ipad), "z_{g}", "1/N_{jet} dN/dz_{g}", 0., 0.6, 0., 10.))->Draw("axis");
        label->Draw();
        TLegend *leg(nullptr);
        TPaveText *jetlabel = nullptr;
        if(ipad == 1){
            leg = new ROOT6tools::TDefaultLegend(0.55, 0.7, 0.89, 0.89);
            leg->Draw();
            jetlabel = new ROOT6tools::TNDCLabel(0.15, 0.8, 0.55, 0.89, Form("%s, R=%.1f, %s", jd.fJetType.data(), jd.fJetRadius, jd.fTrigger.data()));
            jetlabel->Draw();
        }
        defaultstyle.SetStyle<TH1>(*spec.bindata);
        varstyle.SetStyle<TH1>(*varspec);
        spec.bindata->Draw("epsame");
        varspec->Draw("epsame");
        if(ipad == 1){
            leg->AddEntry(spec.bindata, "Default", "lep");
            leg->AddEntry(varspec, vartitle.data(), "lep");
        }

        ratioplot->cd(ipad);
        (new ROOT6tools::TAxisFrame(Form("ratioframe_%d", ipad), "z_{g}", "variation / default", 0., 0.6, 0.5, 1.5))->Draw("axis");
        label->Draw();
        if(ipad == 1) {
            jetlabel->Draw();
            (new ROOT6tools::TNDCLabel(0.65, 0.8, 0.89, 0.89, vartitle.data()))->Draw();
        }
        auto ratiospec = makeRatio(varspec, spec.bindata);
        ratiospec->SetName(Form("ratioDefaultVar_pt%d_%d", int(spec.ptmin), int(spec.ptmax)));
        defaultstyle.SetStyle<TH1>(*ratiospec);
        ratiospec->Draw("epsame");
        ratiohists.push_back(ratiospec);

        barlowplot->cd(ipad);
        (new ROOT6tools::TAxisFrame(Form("barlowframe_%d", ipad), "z_{g}", "(default-variation)/(#sqrt{|#sigma_{var}^{2} - sigma_{default}^{2}|})", 0., 0.6, 0., 10.))->Draw("axis");
        label->Draw();
        if(ipad == 1) {
            jetlabel->Draw();
            (new ROOT6tools::TNDCLabel(0.65, 0.8, 0.89, 0.89, vartitle.data()))->Draw();
        }
        auto barlow = makeBarlow(varspec, spec.bindata);
        barlow->SetName(Form("barlowtest_pt%d_%d", int(spec.ptmin), int(spec.ptmax)));
        barlow->SetLineColor(kBlack);
        barlow->SetFillColor(kRed);
        barlow->Draw("boxsame");
        barlowhists.push_back(barlow);
        ipad++;
    }
    compplot->cd();
    compplot->Update();
    compplot->SaveCanvas(compplot->GetName());
    ratioplot->cd();
    ratioplot->Update();
    ratioplot->SaveCanvas(ratioplot->GetName());
    barlowplot->cd();
    barlowplot->Update();
    barlowplot->SaveCanvas(barlowplot->GetName());

    // Create output rootfile
    std::unique_ptr<TFile> outwriter(TFile::Open(Form("systematics_%s_%s.root", varname.data(), tag.data()), "RECREATE"));
    outwriter->cd();
    for(auto s : spectraDefault) s.bindata->Write();
    for(auto s : spectraVariation) s.bindata->Write();
    for(auto r : ratiohists) r->Write();
    for(auto b : barlowhists) b->Write();
}