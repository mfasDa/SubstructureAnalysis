#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"
#include "../../struct/GraphicsPad.cxx"

TH1 *makeRelStat(const TH1 *in) {
    auto result = static_cast<TH1*>(in->Clone(Form("rel%s", in->GetName())));
    result->SetDirectory(nullptr);
    for(auto b : ROOT::TSeqI(0, result->GetXaxis()->GetNbins())){
        auto val = result->GetBinContent(b+1);
        result->SetBinContent(b+1, 1);
        result->SetBinError(b+1, result->GetBinError(b+1)/val);
    }
    return result;
}

TGraphAsymmErrors *makeRelSys(const TGraphAsymmErrors *in) {
    auto result = new TGraphAsymmErrors;
    for(auto p : ROOT::TSeqI(0, in->GetN())){
        auto val = in->GetY()[p];
        result->SetPoint(p, in->GetX()[p], 1.);
        result->SetPointError(p, in->GetEXlow()[p], in->GetEXhigh()[p], in->GetEYlow()[p]/val, in->GetEYhigh()[p]/val);
    }
    return result;
}

void makeTheoryComparisonNew(const std::string_view spectrumfile, const std::string_view powhegfile, const std::string_view herwigfile) {
    std::unique_ptr<TFile> specreader(TFile::Open(spectrumfile.data(), "READ")),
                           powhegreader(TFile::Open(powhegfile.data(), "READ")),
                           herwigreader(TFile::Open(herwigfile.data(), "READ"));

    auto plot = new ROOT6tools::TSavableCanvas("ComparisonToPowheg", "Comparison to powheg", 1500, 700);
    plot->Divide(5, 2);

    int icol(0);
    for(auto r : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", r));
        plot->cd(icol+1);
        GraphicsPad specpad(gPad);
        specpad.Logy();
        specpad.Margins(0.17, 0.05, -1., 0.05);
        specpad.Frame(Form("specframeR%02d", r), "p_{t} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., 350, 1e-9, 1);
        specpad.FrameTextSize(0.047);
        specpad.Label(0.2, 0.15, 0.45, 0.22, Form("R=%.1f", double(r)/10.));

        TLegend *leg(nullptr);
        if(icol == 0) {
            leg = new ROOT6tools::TDefaultLegend(0.35, 0.6, 0.94, 0.94);
            leg->Draw();
        }

        specreader->cd(rstring.data());
        auto spechist = static_cast<TH1 *>(gDirectory->Get(Form("stat_%s", rstring.data())));
        spechist->SetDirectory(nullptr);
        auto corrUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("correlatedUncertainty_%s", rstring.data()))),
             shapeUncertainty = static_cast<TGraphAsymmErrors *>(gDirectory->Get(Form("shapeUncertainty_%s", rstring.data())));
        Style{kBlack, 20}.SetStyle<TH1>(*spechist);
        spechist->Draw("epsame");
        if(leg) leg->AddEntry(spechist, "ALICE Data", "lep");
        corrUncertainty->SetLineColor(kBlack);
        corrUncertainty->SetFillStyle(0);
        corrUncertainty->Draw("2same");
        if(leg) leg->AddEntry(corrUncertainty, "Correlated uncertainties", "f");
        shapeUncertainty->SetFillColor(kGray);
        shapeUncertainty->SetFillStyle(3001);
        shapeUncertainty->Draw("2same");
        if(leg) leg->AddEntry(shapeUncertainty, "Shape uncertainties", "f");

        auto powhegraw = static_cast<TH1 *>(powhegreader->Get(Form("SpectrumPowheg_%s",rstring.data())));
        auto powhegrebinned = powhegraw->Rebin(spechist->GetXaxis()->GetNbins(), Form("powheg_rebinned_%s", rstring.data()), spechist->GetXaxis()->GetXbins()->GetArray());
        powhegrebinned->Scale(1., "width");
        powhegrebinned->SetDirectory(nullptr);
        Style{kBlue, 24}.SetStyle<TH1>(*powhegrebinned);
        powhegrebinned->Draw("epsame");
        if(leg) leg->AddEntry(powhegrebinned, "POWHEG+PYTHIA6", "lep");

        auto herwigraw = static_cast<TH1 *>(herwigreader->Get(Form("SpectrumHerwig_%s",rstring.data())));
        auto herwigrebinned = herwigraw->Rebin(spechist->GetXaxis()->GetNbins(), Form("powheg_rebinned_%s", rstring.data()), spechist->GetXaxis()->GetXbins()->GetArray());
        herwigrebinned->Scale(1., "width");
        herwigrebinned->SetDirectory(nullptr);
        Style{kRed, 24}.SetStyle<TH1>(*herwigrebinned);
        herwigrebinned->Draw("epsame");
        if(leg) leg->AddEntry(herwigrebinned, "Herwig", "lep");


        plot->cd(icol+6);
        GraphicsPad ratiopad(gPad);
        ratiopad.Margins(0.17, 0.05, -1., 0.05);
        ratiopad.Frame(Form("specframeR%02d", r), "p_{t} (GeV/c)", "MC/Data", 0., 350, 0, 3.);
        ratiopad.FrameTextSize(0.047);
        auto statrel = makeRelStat(spechist);
        Style{kBlack, 20}.SetStyle<TH1>(*statrel);
        auto corrrel = makeRelSys(corrUncertainty), shaperel = makeRelSys(shapeUncertainty);
        corrrel->SetLineColor(kBlack);
        corrrel->SetFillStyle(0);
        corrrel->Draw("2same");
        shaperel->SetFillColor(kGray);
        shaperel->SetFillStyle(3001);
        shaperel->Draw("2same");
        auto powhegratio = static_cast<TH1 *>(powhegrebinned->Clone(Form("ratioPowhegData_%s", rstring.data())));
        powhegratio->SetDirectory(nullptr);
        powhegratio->Divide(spechist);
        Style{kBlue, 24}.SetStyle<TH1>(*powhegratio);
        powhegratio->Draw("epsame");
        auto herwigratio = static_cast<TH1 *>(herwigrebinned->Clone(Form("ratioHerwigData_%s", rstring.data())));
        herwigratio->SetDirectory(nullptr);
        herwigratio->Divide(spechist);
        Style{kRed, 25}.SetStyle<TH1>(*herwigratio);
        herwigratio->Draw("epsame");
        icol++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}