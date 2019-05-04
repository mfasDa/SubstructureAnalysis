#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../struct/GraphicsPad.cxx"
#include "../../struct/ResponseReader.cxx"
#include "../../struct/Ratio.cxx"

void compareProjectionsResponseOutliercut(){
    std::map<std::string, ResponseReader> data;
    std::vector<double> outliercuts = {1,15,2,25,3};
    for(auto c : outliercuts){
        std::stringstream cutname;
        cutname << "outlier" << c;
        std::stringstream filename;
        filename << "correctedSVD_fine_lowpt_" << cutname.str() << ".root";
        data[cutname.str()] = ResponseReader(filename.str().data());
    }
    auto jetradii = data.find("outlier3")->second.getJetRadii();

    auto plotDet = new ROOT6tools::TSavableCanvas("projectionResponseDet_outliercuts", "Projection of the response matrix det. level", 300 * jetradii.size(), 700);
    plotDet->Divide(jetradii.size(), 2);
    auto plotPart = new ROOT6tools::TSavableCanvas("projectionResponsePart_outliercuts", "Projection of the response matrix part. level", 300 * jetradii.size(), 700);
    plotPart->Divide(jetradii.size(), 2);

    std::map<std::string, Style> styles = {
        {"outlier1", {kBlue, 24}},
        {"outlier15", {kRed, 25}},
        {"outlier2", {kGreen, 26}},
        {"outlier25", {kMagenta, 27}},
        {"outlier3", {kBlack, 28}}
    };

    int icol(0);
    for(auto r : jetradii) {
        std::map<std::string, TH1 *> projectionsDet, projectionsPart;
        for(auto [cut, resphandler] : data){
            auto response = resphandler.GetResponseMatrixTruncated(r);  
            response->Scale(1./40e6);
            auto projectionDet = response->ProjectionX(Form("projectionResponseDet_R%02d_%s", int(r*10), cut.data())),
                 projectionPart = response->ProjectionY(Form("projectionResponsePart_R%02d_%s", int(r*10), cut.data()));
            projectionDet->SetDirectory(nullptr);
            projectionDet->Scale(1., "width");
            projectionsDet[cut] = projectionDet;
            projectionPart->SetDirectory(nullptr);
            projectionPart->Scale(1., "width");
            projectionsPart[cut] = projectionPart;
        }
        plotDet->cd(icol+1);
        GraphicsPad specpadDet(gPad);
        gPad->SetLogy();
        specpadDet.Margins(0.15, 0.05, -1., 0.05);
        specpadDet.Frame(Form("specpadPartR%02d", int(r*10.)), "p_{t,det} (GeV/c)", "d#sigma/(dp_{t} d#eta) (mb/(GeV/c))", 0., 250., 1e-9, 100);
        specpadDet.FrameTextSize(0.045);
        specpadDet.Label(0.2, 0.15, 0.45, 0.22, Form("R=%.1f", r));
        if(!icol) specpadDet.Legend(0.5, 0.6, 0.89, 0.89);
        for(auto [cut, spec]: projectionsDet){
            double outliercut = double(std::stoi(cut.substr(7)));
            if(outliercut > 10) outliercut /= 10.;
            specpadDet.Draw<TH1>(spec, styles.find(cut)->second, Form("outliercut = %.1f * p_{t,hard}", outliercut));
        }

        plotDet->cd(icol+jetradii.size()+1);
        GraphicsPad ratiopadDet(gPad);
        ratiopadDet.Margins(0.15, 0.05, -1., 0.05);
        ratiopadDet.Frame(Form("ratiopadDetR%02d",int(r*10.)), "p_{t,det}", "Ratio to 3*p_{t,hard}", 0., 250., 0.5, 1.5);
        ratiopadDet.FrameTextSize(0.045);
        auto ref = projectionsDet.find("outlier3")->second;
        for(auto [cut, spec] : projectionsDet){
            if(cut == "outlier3") continue;
            ratiopadDet.Draw<Ratio>(new Ratio(spec, ref), styles.find(cut)->second);
        }

        plotPart->cd(icol+1);
        GraphicsPad specpadPart(gPad);
        gPad->SetLogy();
        specpadPart.Margins(0.15, 0.05, -1., 0.05);
        specpadPart.Frame(Form("specpadPartR%02d", int(r*10.)), "p_{t,part} (GeV/c)", "d#sigma/(dp_{t} d#eta) (mb/(GeV/c))", 0., 650., 1e-12, 1e-1);
        specpadPart.FrameTextSize(0.045);
        specpadPart.Label(0.2, 0.15, 0.45, 0.22, Form("R=%.1f", r));
        if(!icol) specpadPart.Legend(0.5, 0.6, 0.89, 0.89);
        for(auto [cut, spec]: projectionsPart){
            double outliercut = double(std::stoi(cut.substr(7)));
            if(outliercut > 10) outliercut /= 10.;
            specpadPart.Draw<TH1>(spec, styles.find(cut)->second, Form("outliercut = %.1f * p_{t,hard}", outliercut));
        }

        plotPart->cd(icol+jetradii.size()+1);
        GraphicsPad ratiopadPart(gPad);
        ratiopadPart.Margins(0.15, 0.05, -1., 0.05);
        ratiopadPart.Frame(Form("ratiopadPartR%02d",int(r*10.)), "p_{t,det}", "Ratio to 3*p_{t,hard}", 0., 650., 0.5, 1.5);
        ratiopadPart.FrameTextSize(0.045);
        ref = projectionsPart.find("outlier3")->second;
        for(auto [cut, spec] : projectionsPart){
            if(cut == "outlier3") continue;
            ratiopadPart.Draw<Ratio>(new Ratio(spec, ref), styles.find(cut)->second);
        }
        icol++;
    }

    plotDet->cd();
    plotDet->Update();
    plotDet->SaveCanvas(plotDet->GetName());
    plotPart->cd();
    plotPart->Update();
    plotPart->SaveCanvas(plotPart->GetName());
}