#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

class SpectrumPlot : public ROOT6tools::TSavableCanvas {
	public:
		SpectrumPlot(const char *name, const char *title): ROOT6tools::TSavableCanvas(name, title, 1000, 800), __currentpadid(0), __currentpad(nullptr), __currentlegend(nullptr) {
			Divide(3,2);
		}

		virtual ~SpectrumPlot() {}

		void initSpecPad(int padID, const std::string_view rstring, const std::string_view rtitle, const std::string_view trigger) {
		    __currentpadid = padID;
			this->cd(__currentpadid);
			__currentpad = gPad;
			__currentpad->SetLogy();
			__currentpad->SetLeftMargin(0.15);
			__currentpad->SetRightMargin(0.05);
			(new ROOT6tools::TAxisFrame(Form("Specframe%s%s", rstring.data(), trigger.data()), "p_{t} (GeV/c)", "1/N_{ev} dN/dp_{t} ((GeV/c)^-1)", 0., 200, 1, 1e9))->Draw();
			(new ROOT6tools::TNDCLabel(0.2, 0.15, 0.4, 0.22, rtitle.data()))->Draw();
            (new ROOT6tools::TNDCLabel(0.2, 0.25, 0.4, 0.32, trigger.data()))->Draw();
			__currentlegend = new ROOT6tools::TDefaultLegend(0.4, 0.6, 0.94, 0.89);
			__currentlegend->Draw();
		}

        void initRatioPad(int padID, const std::string_view rstring, const std::string_view trigger) {
		    __currentpadid = padID;
			this->cd(__currentpadid);
			__currentpad = gPad;
			__currentpad->SetLeftMargin(0.15);
			__currentpad->SetRightMargin(0.05);
			(new ROOT6tools::TAxisFrame(Form("Ratioframe%s%s", rstring.data(), trigger.data()), "p_{t} (GeV/c)", "from 2D / corr. rebinned", 0., 200, 0.5, 1.5))->Draw();
            __currentlegend = nullptr;
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

void checkProjections1D(const char *rawfile = "rawsoftdrop.root", const char *observable = "Zg"){
    std::unique_ptr<TFile> reader(TFile::Open(rawfile, "READ"));

    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2"};

    auto style = [](Color_t color, Style_t marker) {
		return [color, marker](auto hist) {
			hist->SetMarkerColor(color);
			hist->SetMarkerStyle(marker);
				hist->SetLineColor(color);
		};
	};
    
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", R)),
                    rtitle(Form("R = %.1f", double(R)/10.));
        reader->cd(rstring.data());
        auto rplot = new SpectrumPlot(Form("ComparisonProjections1D_%s", rstring.data()), Form("Comparison pt projections %s", rtitle.data()));
        reader->cd(rstring.data());
        int icol = 0;
        for(auto trg : triggers){
            auto hspecNoCorrNoRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumNoCorrNoRebin%s_%s_%s", rstring.data(), trg.data(), observable))),
                 hspecNoCorrRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumNoCorrRebin%s_%s_%s", rstring.data(), trg.data(), observable))),
                 hspecCorrNoRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumCorrectedNoRebin%s_%s_%s", rstring.data(), trg.data(), observable))),
                 hspecCorrRebin = static_cast<TH1 *>(gDirectory->Get(Form("jetSpectrumCorrectedRebin%s_%s_%s", rstring.data(), trg.data(), observable)));
            if(hspecNoCorrNoRebin) hspecNoCorrNoRebin->SetDirectory(nullptr);
            if(hspecNoCorrRebin) hspecNoCorrRebin->SetDirectory(nullptr);
            if(hspecCorrNoRebin) hspecCorrNoRebin->SetDirectory(nullptr);
            if(hspecCorrRebin) hspecCorrRebin->SetDirectory(nullptr);
            
            auto reference = hspecCorrRebin;
            if(!reference) reference = hspecNoCorrRebin; // in case of min. bias the corrected spectrum 
            // re-apply projection from 2D histogram
            auto spec2D = static_cast<TH2 *>(gDirectory->Get(Form("h%sVsPt%sCorrected", observable, trg.data())));
            auto specProjectedFrom2D = spec2D->ProjectionY(Form("projectionFrom2D_%s_%s", rstring.data(), trg.data()));  
            specProjectedFrom2D->SetDirectory(nullptr);
            specProjectedFrom2D->Scale(1., "width");

            rplot->initSpecPad(icol+1, rstring, rtitle, trg);
            style(kRed, 24)(hspecNoCorrNoRebin);
            rplot->drawPad(hspecNoCorrNoRebin, "No corr., no rebin.");
            style(kBlue, 25)(hspecNoCorrRebin);
            rplot->drawPad(hspecNoCorrRebin, "No corr., rebin.");
            if(hspecCorrNoRebin){
                style(kGreen, 26)(hspecCorrNoRebin);
                rplot->drawPad(hspecCorrNoRebin, "Corr., no rebin.");
            }
            if(hspecCorrRebin){
                style(kViolet, 27)(hspecCorrRebin);
                rplot->drawPad(hspecCorrRebin, "Corr., rebin.");
            }
            style(kBlack, 20)(specProjectedFrom2D);
            rplot->drawPad(specProjectedFrom2D, "From 2D corrected");

            rplot->initRatioPad(icol+1+3, rstring, trg);
            auto ratio = static_cast<TH1 *>(specProjectedFrom2D->Clone(Form("ratio_%s_%s", rstring.data(), trg.data())));
            ratio->SetDirectory(nullptr);
            ratio->Divide(reference);
            style(kBlack, 20)(ratio);
            rplot->drawPad(ratio, "Ratio");
            icol++;
        }
        rplot->saveCanvas();
    }
}