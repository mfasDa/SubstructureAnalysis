#include "../../meta/stl.C"
#include "../../meta/root.C"
#include "../../meta/root6tools.C"
#include "../../helpers/graphics.C"

struct graphdata {
	TGraphErrors *mean;
	TGraphErrors *median;
	TGraphErrors *resolution;
};

std::map<int, graphdata> readFile(const std::string_view filename) {
	std::map<int, graphdata> result;
	std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
	for(auto R : ROOT::TSeqI(2, 7)) {
		std::string rstring = Form("R%02d", R);
		reader->cd(rstring.data());
		graphdata rbin;
		rbin.mean = gDirectory->Get<TGraphErrors>(Form("meanAll_%s", rstring.data()));
		rbin.median = gDirectory->Get<TGraphErrors>(Form("medianAll_%s", rstring.data()));
		rbin.resolution = gDirectory->Get<TGraphErrors>(Form("widthAll_%s", rstring.data()));
		result[R] = rbin;
	}
	return result;
}

ROOT6tools::TSavableCanvas *makeComparisonPlot(std::map<int, graphdata> &datasameacceptance, std::map<int, graphdata> &datafeedout, int R) {
  const double kPtMin = 0.0,
              kPtMax = 200.;
  auto radius = double(R)/10.;

  auto graphsSameAcceptance = datasameacceptance.find(R),
       graphsFeedout = datafeedout.find(R);
  std::map<std::string, Style> acctypes = {{"Same", {kRed, 24}}, {"Feedout", {kBlue, 25}}};

	std::stringstream plotname, plottitle;
  plotname << "acceptanceComparison_R" << std::setw(2) << std::setfill('0') << R;
  plottitle << "Energy scale (R=" << std::setprecision(1) << radius;
  plottitle << ")";
  auto plot = new ROOT6tools::TSavableCanvas(plotname.str().data(), plottitle.str().data(), 1200,  600);
  plot->Divide(3,1);

  plot->cd(1);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto meanframe = new TH1F("meanframe", "; p_{t,part} (GeV/c); <(p_{t,det} - p_{t,part})/p_{t,part}>", (int)kPtMax, kPtMin, kPtMax);
  meanframe->SetDirectory(nullptr);
  meanframe->SetStats(false);
  meanframe->GetYaxis()->SetRangeUser(-0.5, 0.5);
  meanframe->Draw("axis");

  auto leg = new TLegend(0.4, 0.65, 0.89, 0.89);
  InitWidget(*leg);
  leg->Draw();

  acctypes["Same"].SetStyle(*(graphsSameAcceptance->second.mean));
  graphsSameAcceptance->second.mean->Draw("epsame");
  leg->AddEntry(graphsSameAcceptance->second.mean, "Same acceptance");
  acctypes["Feedout"].SetStyle(*(graphsFeedout->second.mean));
  graphsFeedout->second.mean->Draw("epsame");
  leg->AddEntry(graphsFeedout->second.mean, "Include feedout");

  auto chargelabel = new TPaveText(0.7, 0.5, 0.89, 0.55, "NDC");
  InitWidget(*chargelabel);
  chargelabel->AddText(Form("R=%.1f", radius));
  chargelabel->Draw();

  plot->cd(2);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto medianframe = new TH1F("medianframe", "; p_{t,part} (GeV/c); median((p_{t,det} - p_{t,part})/p_{t,part})",(int)kPtMax, kPtMin, kPtMax);
  medianframe->SetDirectory(nullptr);
  medianframe->SetStats(false);
  medianframe->GetYaxis()->SetRangeUser(-0.5, 0.5);
  medianframe->Draw("axis");;

  acctypes["Same"].SetStyle(*(graphsSameAcceptance->second.median));
  graphsSameAcceptance->second.median->Draw("epsame");
  acctypes["Feedout"].SetStyle(*(graphsFeedout->second.median));
  graphsFeedout->second.median->Draw("epsame");

  plot->cd(3);
  gPad->SetLeftMargin(0.14);
  gPad->SetRightMargin(0.06);
  auto widthframe = new TH1F("widthframe", "; p_{t,part} (GeV/c); #sigma((p_{t,det} - p_{t,part})/p_{t,part})", (int)kPtMax, kPtMin, kPtMax);
  widthframe->SetDirectory(nullptr);
  widthframe->SetStats(false);
  widthframe->GetYaxis()->SetRangeUser(0., .4);
  widthframe->Draw("axis");

  acctypes["Same"].SetStyle(*(graphsSameAcceptance->second.resolution));
  graphsSameAcceptance->second.resolution->Draw("epsame");
  acctypes["Feedout"].SetStyle(*(graphsFeedout->second.resolution));
  graphsFeedout->second.resolution->Draw("epsame");

  plot->cd();
  plot->Update();
  return plot;	
}

void compareEnergyScaleFeedout() {
	std::string basename = "EnergyScaleResults_tc200.root";
	auto dataSameAcceptance = readFile(Form("sameacceptance/%s", basename.data())),
	     dataFeedout = readFile(Form("feedout/%s", basename.data()));

  for(auto R : ROOT::TSeqI(2, 7)) {
    auto plot = makeComparisonPlot(dataSameAcceptance, dataFeedout, R);
    plot->SaveCanvas(plot->GetName());
  }
}