#include "../meta/root.C"
#include "../helpers/math.C"
#include "../unfolding/binnings/binningZg.C"

void extractZgSim(const std::string_view filename){
    ROOT::EnableImplicitMT(8);
    auto binningZg = getZgBinningFine(),
         binningPtMB = getMinBiasPtBinningPart(),
         binningPtEJ2 = getEJ2PtBinningPart(),
         binningPtEJ1 = getEJ1PtBinningPart();
    // Make combined pt binning
    std::vector<double> binningPtCombined;
    for(auto b : binningPtMB) {
        if(b >= 30. && b < 80.) binningPtCombined.push_back(b);
    }
    for(auto b : binningPtEJ2) {
        if(b >= 80. && b < 120.) binningPtCombined.push_back(b);
    }
    for(auto b : binningPtEJ1) {
        if(b >= 120. && b <= 200.) binningPtCombined.push_back(b);
    }
    ROOT::RDataFrame zgframe("shapesnew0", filename);
    std::vector<ROOT::RDF::RResultPtr<TH1D>> zgpt;
    for(auto binID : ROOT::TSeqI(0, binningPtCombined.size()-1)){
        std::stringstream filterstring, histname, histtitle;
        filterstring << "ptJet >= " << binningPtCombined[binID] << " && ptJet < " << binningPtCombined[binID+1];
        histname << "zgpt_" << int(binningPtCombined[binID]) << "_" << int(binningPtCombined[binID+1]);
        histtitle << "Zg for " << std::fixed << std::setprecision(2) << binningPtCombined[binID] << " GeV/c < p_{t} < " << binningPtCombined[binID+1] << " GeV/c";
        zgpt.push_back(zgframe.Filter(filterstring.str()).Histo1D({histname.str().data(), histtitle.str().data(), static_cast<int>(binningZg.size())-1, binningZg.data()}, "zg", "weightPythiaFromPtHard"));
    }
    
    auto rstring = filename.substr(filename.find_first_of("R")+1, 1);
    int rval = std::stoi(static_cast<std::string>(rstring));
    auto mcengine = filename.substr(0, filename.find_first_of("R")-1);
    std::stringstream outfilename;
    outfilename << "zgdists_" << mcengine << "_R" << std::setw(2) << std::setfill('0') << rval << ".root";
    
    std::unique_ptr<TFile> reader(TFile::Open(outfilename.str().data(), "RECREATE"));
    for(auto &dist : zgpt){
        dist->Scale(1./dist->Integral());
        normalizeBinWidth(dist.GetPtr());
        dist->Write();
    } 
}