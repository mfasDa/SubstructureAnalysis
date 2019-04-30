#include "../meta/stl.C"
#include "../meta/root.C"
#include "../meta/roounfold.C"

TH1 *MakeRefolded1D(const TH1 *histtemplate, const char *name, const TH1 *unfolded, const RooUnfoldResponse &response){
	auto refolded = (TH1 *)histtemplate->Clone(name);
	refolded->Sumw2();
	refolded->Reset();

	for(auto binptsmear : ROOT::TSeqI(0, refolded->GetXaxis()->GetNbins())) {
		double effect = 0, error = 0;
		for(auto binpttrue : ROOT::TSeqI(0, unfolded->GetXaxis()->GetNbins())) {
			effect += unfolded->GetBinContent(binpttrue+1) * response(binptsmear, binpttrue);
			error += TMath::Power(unfolded->GetBinError(binpttrue+1) * response(binptsmear, binpttrue), 2);
		}
		refolded->SetBinContent(binptsmear + 1, effect);
		refolded->SetBinError(binptsmear +1, TMath::Sqrt(error));
	}
	return refolded;
}

void simpleUnfoldingCheckerBayes(const char *inputfile, const char *outputfile){
	std::unique_ptr<TFile> reader(TFile::Open(inputfile, "READ")),
						   writer(TFile::Open(outputfile, "RECREATE"));
    RooUnfold::ErrorTreatment errorTreatment = RooUnfold::kCovToy;
	for(auto r = 0.2; r <= 0.6; r+= 0.1){
		std::string rstring(Form("R%02d", int(r*10.)));
		reader->cd(rstring.data());
		auto basedir = gDirectory;
		basedir->cd("rawlevel");
		auto hraw = (TH1 *)gDirectory->Get(Form("hraw_%s", rstring.data()));
		hraw->SetDirectory(nullptr);
		basedir->cd("response");
		auto response = (TH2 *)gDirectory->Get(Form("Rawresponse_%s_fine_rebinned_standard", rstring.data()));
		response->SetDirectory(nullptr);
		auto truefull = (TH1 *)gDirectory->Get(Form("truefull_%s", rstring.data()));
		truefull->SetDirectory(nullptr);
		basedir->cd("closuretest");
		auto detclosure = (TH1 *)gDirectory->Get("detclosure"),
		     partclosure = (TH1 *)gDirectory->Get("partclosure"),
		     priorsclosure = (TH1 *)gDirectory->Get("priorsclosure");
		auto responseclosure = (TH2 *)gDirectory->Get("responseClosureRebinned");

		detclosure->SetDirectory(nullptr);
		partclosure->SetDirectory(nullptr);
		priorsclosure->SetDirectory(nullptr);
		responseclosure->SetDirectory(nullptr);

		writer->mkdir(rstring.data());
		writer->cd(rstring.data());
		basedir = gDirectory;

		RooUnfoldResponse rresponse(nullptr, truefull, response);
		RooUnfoldResponse rresponseClosure(nullptr, priorsclosure, responseclosure);
		const double kSizeEmcalPhi = 1.88, 
			     kSizeEmcalEta = 1.4;
		auto acceptance = (kSizeEmcalPhi - 2 * r) * (kSizeEmcalEta - 2 * r) / (TMath::TwoPi());
		for(auto ireg: ROOT::TSeqI(1,11)){
			RooUnfoldBayes unfolder(&rresponse, hraw, 4);
		        auto specunfolded = unfolder.Hreco(errorTreatment);
			specunfolded->SetDirectory(nullptr);
			specunfolded->SetName(Form("unfolded_reg%d", ireg));
			auto specrefolded = MakeRefolded1D(hraw, Form("backfolded_reg%d", ireg), specunfolded, rresponse);
			auto specnormalized = (TH1 *)specunfolded->Clone(Form("normalized_reg%d", ireg));
			specnormalized->Scale(1./acceptance);

            RooUnfoldBayes unfolderClosure(&rresponseClosure, detclosure, ireg);
			auto specunfoldedClosure = unfolderClosure.Hreco(errorTreatment);
			specunfoldedClosure->SetDirectory(nullptr);
			specunfoldedClosure->SetName(Form("unfoldedClosure_reg%d", ireg));

			specunfolded->Scale(1., "width");
			specnormalized->Scale(1., "width");
			specunfoldedClosure->Scale(1., "width");

			basedir->mkdir(Form("reg%d", ireg));
			basedir->cd(Form("reg%d", ireg));
			specunfolded->Write();
			specrefolded->Write();
			specnormalized->Write();
			specunfoldedClosure->Write();
		}

		basedir->mkdir("rawlevel");
		basedir->cd("rawlevel");
		hraw->Write();
		basedir->mkdir("response");
		basedir->cd("response");
		response->Write();
		truefull->Write();
		basedir->mkdir("closuretest");
		basedir->cd("closuretest");
		detclosure->Write("detclosure");
		partclosure->Write("partclosure");
		priorsclosure->Write("priorsclosure");
		responseclosure->Write("responseClosureRebinned");
	}

}
