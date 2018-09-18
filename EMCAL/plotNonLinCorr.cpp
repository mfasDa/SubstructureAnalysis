#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"

void plotNonLinCorr() {
    std::vector<std::array<double, 4>> data = {{{0.430729, 0.002090, 0.897914, 0.023202}}, {{0.683633, 0.002061, 0.936868, 0.014771}}, 
                                               {{0.937881, 0.001964, 0.957314, 0.014032}}, {{1.192216, 0.002687, 0.969518, 0.013759}},
                                               {{1.451105, 0.002885, 0.980675, 0.013557}}, {{1.968016, 0.003185, 0.994098, 0.013338}},
                                               {{3.020276, 0.003624, 1.013618, 0.013167}}, {{4.008653, 0.007320, 1.007275, 0.013190}},
                                               {{5.013564, 0.007574, 1.006800, 0.013128}}, {{6.047092, 0.011562, 1.011270, 0.013170}},
                                               {{6.000303, 0.001254, 1.003445, 0.010037}}, {{8.061410, 0.002012, 1.010240, 0.010023}},
                                               {{10.072618, 0.001192, 1.009311, 0.010013}}, {{15.089874, 0.001732, 1.007355, 0.010006}},
                                               {{25.157666, 0.002585, 1.007124, 0.010003}}, {{50.578486, 0.004473, 1.011981, 0.010001}},
                                               {{75.068898, 0.008564, 1.001190, 0.010001}}, {{99.561576, 0.011197, 0.995818, 0.010001}},
                                               {{123.421301, 0.018420, 0.987531, 0.010001}}, {{144.205121, 0.020939, 0.961498, 0.010001}},
                                               {{162.661916, 0.022827, 0.929604, 0.010001}}, {{180.211794, 0.059450, 0.901150, 0.010004}},
                                               {{197.625451, 0.207522, 0.878415, 0.010042}}};
    TGraphErrors *dp = new TGraphErrors;
    dp->SetMarkerStyle(20);
    dp->SetLineColor(kBlack);
    dp->SetMarkerColor(kBlack);
    int np(0);
    for(auto d : data) {
        dp->SetPoint(np, d[0], d[2]);
        dp->SetPointError(np, d[1], d[3]);
        np++;
    }

    std::array<double, 7> fNonLinearityParams = {{0.976941, 0.162310, 1.08689, 0.0819592, 152.338, 30.9594, 0.9615}};
    auto model = [fNonLinearityParams](double *x, double *p) { return 1/(fNonLinearityParams[6]/(fNonLinearityParams[0]*(1./(1.+fNonLinearityParams[1]*TMath::Exp(-x[0]/fNonLinearityParams[2]))*1./(1.+fNonLinearityParams[3]*TMath::Exp((x[0]-fNonLinearityParams[4])/fNonLinearityParams[5]))))); };
    auto nonlincorr = new TF1("nonlinearity", model, 0., 200., 1);
    auto plotlin = new ROOT6tools::TSavableCanvas("nonlincorrlin", "Non-linearity correction (lin. scale)", 800, 600);
    plotlin->cd();
    (new ROOT6tools::TAxisFrame("nonlinframelin", "E_{cl} (GeV/c)", "Non-linearity correction", 0., 200., 0., 1.1))->Draw("axis");
    nonlincorr->SetLineColor(kRed);
    nonlincorr->Draw("lsame");
    dp->Draw("epsame");
    plotlin->SaveCanvas(plotlin->GetName());

    auto plotlog = new ROOT6tools::TSavableCanvas("nonlincorrlog", "Non-linearity correction (log. scale)", 800, 600);
    plotlog->cd();
    gPad->SetLogx();
    (new ROOT6tools::TAxisFrame("nonlinframelog", "E_{cl} (GeV/c)", "Non-linearity correction", 0.2, 200., 0., 1.1))->Draw("axis");
    nonlincorr->Draw("lsame");
    dp->Draw("epsame");
    plotlog->SaveCanvas(plotlog->GetName());
}