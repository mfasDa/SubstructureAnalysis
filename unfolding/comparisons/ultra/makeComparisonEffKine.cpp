#include "../../../meta/stl.C"
#include "../../../meta/root.C"
#include "../../../meta/root6tools.C"
#include "../../../helpers/graphics.C"
#include "../../../struct/GraphicsPad.cxx"
#include "../../../struct/Ratio.cxx"

std::map<double, TH1 *> readFile(const std::string_view name) {
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(name.data(), "READ"));
    for(auto r = 0.2; r <= 0.6; r+= 0.1) {
        std::string rstring(Form("R%02d", int(r*10.)));
        reader->cd(rstring.data());
        gDirectory->cd("response"); 
        auto hist = static_cast<TH1 *>(gDirectory->Get(Form("effKine_%s", rstring.data())));
        hist->SetDirectory(nullptr);
        result[r] = hist;
    } 
    return result;
}

void makeComparisonEffKine(int ultraoption){
    std::map<std::string, std::map<double, TH1 *>> data;
    std::vector<std::string> variations = {"tc200", "tc250", "notc"};
    for(const auto &v : variations) data[v] = readFile(Form("correctedSVD_ultra%d_%s.root", ultraoption, v.data()));
    std::map<std::string, Style> styles = {{"tc200", {kBlack, 20}}, {"tc250", {kBlue, 21}}, {"notc", {kRed, 22}}};

    auto nradii = data["tc200"].size();
    auto plot = new ROOT6tools::TSavableCanvas(Form("effKineCompUltra%d", ultraoption), Form("Kinematic efficiency for jets with pt < %d GeV/c", ultraoption), 300 * nradii , 700);
    plot->Divide(nradii, 2);
    int icol = 0;
    for(auto r = 0.2; r <= 0.6; r+=0.1){
        plot->cd(icol + 1);
        GraphicsPad specpad(gPad);
        specpad.Frame(Form("specframe%02d", int(r*10.)), "p_{t,j,part} (GeV/c)", "#epsilon_{kine}", 0., 400., 0., 1.1);
        specpad.FrameTextSize(0.05);
        specpad.Margins(0.17, 0.04, 0.12, 0.04);
        specpad.Label(0.2, 0.3, 0.45, 0.37, Form("R = %.1f", r));
        if(icol == 0){
            specpad.Legend(0.55, 0.3, 0.89, 0.6);
            specpad.Label(0.2, 0.15, 0.89, 0.25, Form("10 GeV/c < p_{t,j,det} < %d GeV/c", ultraoption));
        }
        TH1 *reference(nullptr);
        std::map<std::string, TH1 *> variations;
        for(const auto v : data) {
            auto eff = v.second.find(r)->second;
            specpad.Draw<TH1>(eff, styles[v.first], v.first);
            if(v.first == "tc200") reference = eff;
            else variations[v.first] = eff;
        }

        plot->cd(icol + nradii + 1);
        GraphicsPad ratiopad(gPad);
        ratiopad.Frame(Form("ratioframe%02d", int(r*10.)), "p_{t,j,part} (GeV/c)", "#epsilon_{kine} (var) / #epsilon_{kine} (t,c < 200 GeV/c)", 0., 400., 0.8, 1.2);
        ratiopad.FrameTextSize(0.05);
        ratiopad.Margins(0.17, 0.04, 0.12, 0.04);
        for(auto v : variations){
            std::cout << "Doing variation " << v.first << std::endl;
            auto ratio = new Ratio(v.second, reference);
            ratiopad.Draw<Ratio>(ratio, styles[v.first], "");
        } 
        icol += 1;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}