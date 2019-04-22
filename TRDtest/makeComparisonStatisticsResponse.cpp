#include "../helpers/graphics.C"
#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../struct/Rebinner.cxx"
#include "../struct/GraphicsPad.cxx"
#include "../struct/ResponseReader.cxx"
#include "../unfolding/binnings/binningPt1D.C"

void makeComparisonStatisticsResponse(){
    ResponseReader readerWith("withTRD/correctedSVD_ultra240_tc250.root"),
                   readerWithout("withoutTRD/correctedSVD_ultra240_tc250.root");
    Rebinner rebinner(getJetPtBinningNonLinTrueUltra240());
    auto jetradii = readerWith.getJetRadii();

    auto plot = new ROOT6tools::TSavableCanvas("responseProjection_200_240", "Projection response matrix 200 - 240 GeV/c", 1200, 700);
    plot->DivideSquare(jetradii.size());

    Style stylewith{kRed, 24}, stylewithout{kBlue, 25};
    int ipad = 1;
    for(auto r : jetradii) {
        std::string rstring(Form("R%02d", int(r*10.)));
        plot->cd(ipad);
        auto responsewith = readerWith.GetResponseMatrixFine(r),
             responsewithout = readerWithout.GetResponseMatrixFine(r);
        responsewith->Scale(1./(40e6));     // undo artificial scaling of the response matrix
        responsewithout->Scale(1./(40e6));

        int bin200 = responsewith->GetXaxis()->FindBin(200.5);
        int bin240 = responsewith->GetXaxis()->FindBin(239.5);
        auto ptmin = responsewith->GetXaxis()->GetBinLowEdge(bin200),
             ptmax = responsewith->GetXaxis()->GetBinUpEdge(bin240);
        auto specpartwith = rebinner(responsewith->ProjectionY(Form("specPartWith_%s", rstring.data()), bin200, bin240)),
             specpartwithout = rebinner(responsewithout->ProjectionY(Form("specPartWithout_%s", rstring.data()), bin200, bin240));
        specpartwith->SetDirectory(nullptr);
        specpartwithout->SetDirectory(nullptr);
        specpartwith->Scale(1., "width");
        specpartwithout->Scale(1., "width");

        GraphicsPad specpad(gPad);
        specpad.Margins(0.14, 0.04, -1., 0.04);
        specpad.Logy();
        specpad.Frame(Form("specframe_%s", rstring.data()), "p_{t} (GeV/c)", "d#sigma/(dp_{t}d#eta) (mb/(GeV/c))", 0., 500., 1e-14, 1e-4);
        specpad.Label(0.25, 0.75, 0.45, 0.82, Form("R = %.1f", r));
        if(ipad == 1){
            specpad.Legend(0.5, 0.55, 0.94, 0.85);
            specpad.Label(0.2, 0.85, 0.94, 0.94, Form("%.1f GeV/c < p_{t,jet,det} < %.1f GeV/c", ptmin, ptmax));
        }  
        specpad.Draw<TH1>(specpartwith, stylewith, "With TRD");
        specpad.Draw<TH1>(specpartwithout, stylewithout, "Without TRD");
        ipad++;
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}