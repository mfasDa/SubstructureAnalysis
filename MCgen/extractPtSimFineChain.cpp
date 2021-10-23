#include "../meta/root.C"
#include "../helpers/math.C"
#include "../unfolding/binnings/binningPt1D.C"

void extractPtSimFineChain(const std::string filelist){
    ROOT::EnableImplicitMT(8);

    std::vector<std::string> files;
    std::ifstream listreader(filelist);
    std::string tmpfile;
    while(std::getline(listreader, tmpfile)) files.push_back(tmpfile);
    for(auto &f : files) std::cout << f << std::endl;
    std::string teststring = files[0];
    teststring.erase(teststring.find("converted_mfasel"), strlen("converted_mfasel"));
    auto rstring = teststring.substr(teststring.find_first_of("r")+1, 1);
    int rval = std::stoi(static_cast<std::string>(rstring));
    double radius = double(rval) / 10.;
    auto mcengine = "Perugia11";// files[0].substr(0, files[0].find_first_of("R"));
    
    std::vector<double> binningpart;
    for(auto b : ROOT::TSeqI(0, 301)) binningpart.emplace_back(b);

    //ROOT::RDataFrame specframe("shapesnew0", inputfile);
    ROOT::RDataFrame specframe("newtree", files);
    auto spectrum = specframe.Histo1D({"jetptspectrum", "Jet p_{t} spectrum", static_cast<int>(binningpart.size())-1, binningpart.data()}, "ptJet", "weightPythiaFromPtHard");
    normalizeBinWidth(spectrum.GetPtr());
    double acceptancecorr = 1.8 - 2 * radius;
    spectrum->Scale(1./acceptancecorr);

    std::stringstream outfilename;
    outfilename << "spectrumFine" << mcengine << "_R" << std::setw(2) << std::setfill('0') << rval << ".root";

    std::unique_ptr<TFile> reader(TFile::Open(outfilename.str().data(), "RECREATE"));
    reader->cd();
    spectrum->Write();
}