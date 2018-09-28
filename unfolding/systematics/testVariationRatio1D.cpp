#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/filesystem.C"
#include "../../helpers/graphics.C"
#include "../../helpers/math.C"
#include "../../helpers/root.C"
#include "../../helpers/string.C"

struct settings1D{
    std::string unfoldingmethod;
    double radius;
};

TH1 *getCorrected(const std::string_view filename, const std::string_view varname, double radius, int regularization  = 4) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    auto keys = CollectionToSTL<TKey>(reader->GetListOfKeys());
    std::string dirname;
    std::array<std::string, 2> regnames = {{"iteration", "regularization"}};
    for(const auto &n : regnames) {
        std::stringstream regnamebuilder;
        regnamebuilder << n << regularization;
        auto regstring = regnamebuilder.str();
        auto found = std::find_if(keys.begin(), keys.end(), [&regstring](const TKey *k) { return regstring == k->GetName(); });
        if(found != keys.end()) {
            dirname = (*found)->GetName();
            break;
        }
    }
    if(!dirname.length()) return nullptr;
    reader->cd(dirname.data());
    auto histos = CollectionToSTL<TKey>(gDirectory->GetListOfKeys());
    auto histunfolded = std::find_if(histos.begin(), histos.end(), [](const TKey *k) { return contains(k->GetName(), "normalized") && !contains(k->GetName(), "Closure"); });
    if(histunfolded == histos.end()) {
        std::cout << "no hist found" << std::endl;
        return nullptr;
    }
    auto hist = (*histunfolded)->ReadObject<TH1>();
    hist->SetDirectory(nullptr);
    normalizeBinWidth(hist);
    return hist;
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

std::string get1DFileTag(const std::string_view filename) {
    auto res = basename(filename);
    res.erase(res.find(".root"), 5);
    res.erase(res.find("corrected1D"), 11);
    return res;
}

settings1D getUnfoldingSettings(const std::string_view filetag){
    auto tokens = tokenize(std::string(filetag), '_');
    double radius = double(std::stoi(tokens[1].substr(1)))/10.;
    return {tokens[0], radius};
}

void testVariationRatio1D(const std::string_view varname, 
                          const std::string_view vartitle, 
                          const std::string_view defaultfilenum, 
                          const std::string_view defaultfileden,  
                          const std::string_view varfilenum, 
                          const std::string_view varfileden, 
                          int regdefault = 4, int regvar = 4){
    auto specDefaultNum = getCorrected(defaultfilenum, "defaultNumerator", regdefault), specDefaultDen = getCorrected(defaultfileden, "defaultDenominator", regdefault),
         specVariationNum = getCorrected(varfilenum, "variationNumerator", regvar), specVariationDen = getCorrected(varfileden, "variationDenominator", regvar);
    
    auto ratioDefault = histcopy(specDefaultNum), ratioVariation = histcopy(specVariationNum);
    ratioDefault->SetDirectory(nullptr);
    ratioVariation->SetDirectory(nullptr);
    ratioDefault->Divide(specDefaultNum, specDefaultDen, 1., 1., "b");
    ratioVariation->Divide(specVariationNum, specVariationDen, 1., 1., "b");

    auto tagdefaultNum = get1DFileTag(defaultfilenum), 
         tagdefaultDen = get1DFileTag(defaultfileden), 
         tagvariationNum = get1DFileTag(varfilenum),
         tagvariationDen = get1DFileTag(varfileden);
    std::cout << tagdefaultNum << ", " << tagdefaultDen << ", " << tagvariationNum << ", " << tagvariationDen << std::endl;
    auto uddefaultNum = getUnfoldingSettings(tagdefaultNum), 
         uddefaultDen = getUnfoldingSettings(tagdefaultDen),
         udvariationNum = getUnfoldingSettings(tagvariationNum),
         udvariationDen = getUnfoldingSettings(tagvariationDen);
    auto compplot = new ROOT6tools::TSavableCanvas(Form("compRatio%s_%s_R%02dR%02d", uddefaultNum.unfoldingmethod.data(), varname.data(), int(uddefaultNum.radius*10.), int(uddefaultDen.radius*10.)), Form("Comparison R=%.1f/%.1f %s %s", uddefaultNum.radius, uddefaultDen.radius, varname.data(), uddefaultNum.unfoldingmethod.data()), 1200, 1000);
    compplot->Divide(3,1);
    Style defaultstyle{kRed, 24}, varstyle{kBlue, 25};

    compplot->cd(1);
    (new ROOT6tools::TAxisFrame("compframe", "p_{t} (GeV/c)", Form("d#sigam(R=%.1f)/dp_{t}d#eta)/(d#sigma(R=%.1f)/dp_{t}d#eta)", uddefaultNum.radius, uddefaultDen.radius), 0., 250., 0., 1.))->Draw("axis");
    TLegend *leg = new ROOT6tools::TDefaultLegend(0.55, 0.7, 0.89, 0.89);
    leg->Draw();

    defaultstyle.SetStyle<TH1>(*ratioDefault);
    varstyle.SetStyle<TH1>(*ratioVariation);
    ratioDefault->Draw("epsame");
    ratioVariation->Draw("epsame");
    leg->AddEntry(ratioDefault, "Default", "lep");
    leg->AddEntry(ratioVariation, vartitle.data(), "lep");

    compplot->cd(2);
    (new ROOT6tools::TAxisFrame("ratioframe", "p_{t} (GeV/c)", "variation / default", 0., 250., 0.5, 1.5))->Draw("axis");
    auto ratioRatios = makeRatio(ratioVariation, ratioDefault);
    ratioRatios->SetNameTitle("ratioDefaultVar", "Ratio default variation");
    defaultstyle.SetStyle<TH1>(*ratioRatios);
    ratioRatios->Draw("epsame");

    compplot->cd(3);
    (new ROOT6tools::TAxisFrame("barlowframe_%d", "p_{t} (GeV/c)", "(default-variation)/(#sqrt{|#sigma_{var}^{2} - sigma_{default}^{2}|})", 0., 250., 0., 10.))->Draw("axis");
    auto barlow = makeBarlow(ratioVariation, ratioDefault);
    barlow->SetNameTitle("barlowtest", "Barlow test");
    barlow->SetLineColor(kBlack);
    barlow->SetFillColor(kRed);
    barlow->Draw("boxsame");

    compplot->cd();
    compplot->Update();
    compplot->SaveCanvas(compplot->GetName());

    // Create output rootfile
    std::unique_ptr<TFile> outwriter(TFile::Open(Form("systematicsR%02dR%02d_%s_%s.root", int(uddefaultNum.radius*10.), int(uddefaultDen.radius*10.),varname.data(), uddefaultNum.unfoldingmethod.data()), "RECREATE"));
    outwriter->cd();
    ratioDefault->Write("DefaultRatio");
    ratioVariation->Write("VariationRatio");
    ratioRatios->Write("RatioRatios");
    barlow->Write("barlowtest");
}