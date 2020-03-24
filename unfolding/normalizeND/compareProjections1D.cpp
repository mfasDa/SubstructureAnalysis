#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

class SpectrumPlot : public ROOT6tools::TSavableCanvas {
	public:
		SpectrumPlot(const char *name, const char *title): ROOT6tools::TSavableCanvas(name, title, 1000, 800), __currentpadid(0), __currentpad(nullptr), __currentlegend(nullptr) {
			Divide(3,2);
		}

		virtual ~SpectrumPlot() {}

		void initPad(const std::string_view rstring, const std::string_view rtitle, const std::string_view tag) {
			__currentpadid++;
			this->cd(__currentpadid);
			__currentpad = gPad;
			__currentpad->SetLogy();
			__currentpad->SetLeftMargin(0.15);
			__currentpad->SetRightMargin(0.05);
			(new ROOT6tools::TAxisFrame(Form("Specframe%s%s", rstring.data(), tag.data()), "p_{t} (GeV/c)", "1/N_{ev} dN/dp_{t} ((GeV/c)^-1)", 0., 200, 1, 1e9))->Draw();
			(new ROOT6tools::TNDCLabel(0.2, 0.15, 0.4, 0.22, rtitle.data()))->Draw();
			if(__currentpadid == 1) {
				__currentlegend = new ROOT6tools::TDefaultLegend(0.7, 0.6, 0.94, 0.89);
				__currentlegend->Draw();
			} else __currentlegend = nullptr;
		}

		void drawPad(TH1 *hist, const std::string_view legentry) {
			__currentpad->cd();
			hist->Draw("epsame");
			if(__currentlegend) __currentlegend->AddEntry(hist, legentry.data(), "lepsame");
		}

		void saveCanvas() {
			this->cd();
			this->Update();
			this->SaveCanvas(this->GetName());
		}

	private:
		int 				__currentpadid;
		TVirtualPad 		*__currentpad;
		TLegend 			*__currentlegend;
};

void compareProjections1D(const char *inputfile = "rawsoftdrop.root", const char *observable = "Zg"){
    std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ"));

    const std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};

	auto style = [](Color_t color, Style_t marker) {
		return [color, marker](auto hist) {
			hist->SetMarkerColor(color);
			hist->SetMarkerStyle(marker);
				hist->SetLineColor(color);
		};
	};
	auto makestyle = [style](const std::string_view trigger) {
		std::map<std::string, Color_t> colors = {{"INT7", kBlack}, {"EJ1", kRed}, {"EJ2", kBlue}};
		std::map<std::string, Style_t> markers = {{"INT7", 20}, {"EJ1", 24}, {"EJ2", 25}};
		return style(colors[trigger.data()], markers[trigger.data()]);
	};

    auto plotNoCorrNoRebin = new SpectrumPlot(Form("comparisonProjections1D%sNoCorrNoRebin", observable), Form("Comparison 1D projections1D (no correction, no rebin, %s)", observable)),
         plotNoCorrRebin   = new SpectrumPlot(Form("comparisonProjections1D%sNoCorrRebin", observable), Form("Comparison 1D projections1D (no correction, rebin, %s)", observable)),
         plotCorrNoRebin = new SpectrumPlot(Form("comparisonProjections1D%sCorrNoRebin", observable), Form("Comparison 1D projections1D (with correction, no rebin, %s)", observable)),
         plotCorrRebin   = new SpectrumPlot(Form("comparisonProjections1D%sCorrRebin", observable), Form("Comparison 1D projections1D (with correction, rebin, %s)", observable));

    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", R)),
                    rtitle(Form("R = %.1f", double(R)/10.));
        plotNoCorrNoRebin->initPad(rstring, rtitle, "NoCorrNoRebin"); 
        plotNoCorrRebin->initPad(rstring, rtitle, "NoCorrRebin"); 
        plotCorrNoRebin->initPad(rstring, rtitle, "CorrNoRebin"); 
        plotCorrRebin->initPad(rstring, rtitle, "CorrRebin"); 
        reader->cd(rstring.data());

        for(auto trg : triggers) {
            auto trgstyle = makestyle(trg);
            auto specNoCorrNoRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumNoCorrNoRebin%s_%s_%s", rstring.data(), trg.data(), observable))),
                 specNoCorrRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumNoCorrRebin%s_%s_%s", rstring.data(), trg.data(), observable))),
                 specCorrNoRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumCorrectedNoRebin%s_%s_%s", rstring.data(), trg.data(), observable))),
                 specCorrRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumCorrectedRebin%s_%s_%s", rstring.data(), trg.data(), observable)));
            specNoCorrNoRebin->SetDirectory(nullptr);
            specNoCorrRebin->SetDirectory(nullptr);
            trgstyle(specNoCorrNoRebin);
            plotNoCorrNoRebin->drawPad(specNoCorrNoRebin, trg.data());
            trgstyle(specNoCorrRebin);
            plotNoCorrRebin->drawPad(specNoCorrRebin, trg); 
            if(specCorrNoRebin) {
                specCorrNoRebin->SetDirectory(nullptr);
                trgstyle(specCorrNoRebin);
                plotCorrNoRebin->drawPad(specCorrNoRebin, trg);
            } else {
                std::cout << "No corrected spectrum for trigger " << trg << std::endl;
                plotCorrNoRebin->drawPad(specNoCorrNoRebin, trg);
            } 
            if(specCorrRebin){
                specCorrRebin->SetDirectory(nullptr);
                trgstyle(specCorrRebin);
                plotCorrRebin->drawPad(specCorrRebin, trg);
            } else{
                std::cout << "No corrected spectrum for trigger " << trg << std::endl;
                plotCorrRebin->drawPad(specNoCorrRebin, trg);
            } 
        }
    }
    plotNoCorrNoRebin->saveCanvas();
    plotNoCorrRebin->saveCanvas();
    plotCorrNoRebin->saveCanvas();
    plotCorrRebin->saveCanvas();
}