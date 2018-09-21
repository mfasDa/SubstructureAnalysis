#include "../meta/root.C"
#include "../helpers/math.C"
#include "../unfolding/binnings/binningPt1D.C"

void extractPtSim(const std::string_view inputfile){
    ROOT::EnableImplicitMT(8);
    auto rstring = inputfile.substr(inputfile.find_first_of("R")+1, 1);
    int rval = std::stoi(static_cast<std::string>(rstring));
    double radius = double(rval) / 10.;
    auto mcengine = inputfile.substr(0, inputfile.find_first_of("R"));
    auto binningpart = getJetPtBinningNonLinTrueLarge();

    //ROOT::RDataFrame specframe("shapesnew0", inputfile);
    ROOT::RDataFrame specframe("newtree", inputfile);
    auto spectrum = specframe.Histo1D({"jetptspectrum", "Jet p_{t} spectrum", static_cast<int>(binningpart.size())-1, binningpart.data()}, "ptJet", "weightPythiaFromPtHard");
    normalizeBinWidth(spectrum.GetPtr());
    double acceptancecorr = 1.8 - 2 * radius;
    spectrum->Scale(1./acceptancecorr);

    std::stringstream outfilename;
    outfilename << "spectrum" << mcengine << "_R" << std::setw(2) << std::setfill('0') << rval << ".root";

    std::unique_ptr<TFile> reader(TFile::Open(outfilename.str().data(), "RECREATE"));
    reader->cd();
    spectrum->Write();
}