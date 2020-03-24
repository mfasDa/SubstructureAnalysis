#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"

class SpectrumPlot : public ROOT6tools::TSavableCanvas {
	public:
		SpectrumPlot(const char *name, const char *title): ROOT6tools::TSavableCanvas(name, title, 1200, 800), __currentpadid(0), __currentpad(nullptr), __currentlegend(nullptr) {
			Divide(5,2);
		}

		virtual ~SpectrumPlot() {}

		void initSpecPad(int padID, const std::string_view rstring, const std::string_view rtitle, bool doLegend) {
			__currentpadid = padID;
			this->cd(__currentpadid);
			__currentpad = gPad;
			__currentpad->SetLogy();
			__currentpad->SetLeftMargin(0.15);
			__currentpad->SetRightMargin(0.05);
			(new ROOT6tools::TAxisFrame(Form("Specframe%s", rstring.data()), "p_{t} (GeV/c)", "1/N_{ev} dN/dp_{t} ((GeV/c)^-1)", 0., 200, 1, 1e9))->Draw();
			(new ROOT6tools::TNDCLabel(0.2, 0.15, 0.4, 0.22, rtitle.data()))->Draw();
			if(doLegend) {
				__currentlegend = new ROOT6tools::TDefaultLegend(0.5, 0.6, 0.94, 0.89);
                __currentlegend->SetTextSize(0.04);
				__currentlegend->Draw();
			} else __currentlegend = nullptr;
		}

        void initRatioPad(int padID, const std::string_view rstring) {
			__currentpadid = padID;
			this->cd(__currentpadid);
			__currentpad = gPad;
			__currentpad->SetLeftMargin(0.15);
			__currentpad->SetRightMargin(0.05);
			(new ROOT6tools::TAxisFrame(Form("Ratioframe%s", rstring.data()), "p_{t} (GeV/c)", "Trigger / Min. Bias", 0., 200, 0., 2.))->Draw();
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

void checkCombinedDistributions(const char *inputfile = "combinedsoftdrop.root", const char *observable = "Zg") {
    std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ"));

    std::vector<std::string> triggers = {"INT7", "EJ1", "EJ2", "combined"};

    auto style = [](Color_t color, Style_t marker) {
		return [color, marker](auto hist) {
			hist->SetMarkerColor(color);
			hist->SetMarkerStyle(marker);
				hist->SetLineColor(color);
		};
	};
    
    auto makestyle = [style](const std::string_view trigger) {
		std::map<std::string, Color_t> colors = {{"INT7", kViolet}, {"EJ1", kRed}, {"EJ2", kBlue}, {"combined", kBlack}};
		std::map<std::string, Style_t> markers = {{"INT7", 24}, {"EJ1", 25}, {"EJ2", 26}, {"combined", 20}};
		return style(colors[trigger.data()], markers[trigger.data()]);
	};

    auto plot = new SpectrumPlot("ComparisonProjections1DCombined", "Comparison 1D distributions from 2D corrected");

    int icol = 0;
    for(auto R : ROOT::TSeqI(2, 7)) {
        std::string rstring(Form("R%02d", R)),
                    rtitle(Form("R = %.1f", double(R)/10.));
        reader->cd(rstring.data());
        plot->initSpecPad(icol + 1, rstring, rtitle, icol == 0);
        std::map<std::string, TH1 *> spectratriggers;
        TH1 *refspectrum(nullptr);
        for(auto trg : triggers) {
            auto dist2D = trg == "combined" ? static_cast<TH2 *>(gDirectory->Get(Form("h%sVsPtRawCombined", observable))) : static_cast<TH2 *>(gDirectory->Get(Form("h%sVsPt%sCorrected", observable, trg.data())));
            auto spectrum = dist2D->ProjectionY(Form("Spectrum%s_%s", trg.data(), rstring.data()));
            spectrum->SetDirectory(nullptr);
            spectrum->Scale(1., "width");
            makestyle(trg)(spectrum);
            plot->drawPad(spectrum, trg);
            if(trg == "INT7") refspectrum = spectrum;
            else spectratriggers[trg] = spectrum;
        }

        plot->initRatioPad(icol+1+5, rstring);
        for(auto &[trg, spec] : spectratriggers) {
            auto ratio = static_cast<TH1 *>(spec->Clone(Form("ratio_%s_INT7_%s", trg.data(), rstring.data())));
            ratio->SetDirectory(nullptr);
            ratio->Divide(refspectrum);
            makestyle(trg)(ratio);
            plot->drawPad(ratio, "RATIO");
        }
        icol++;
    }
    plot->saveCanvas();
}