void convertcompression(const std::string_view inputfile){
    std::string outname = std::string(inputfile);
    outname.erase(outname.find(".root"),5);
    outname += "_oldcompression.root";
    std::unique_ptr<TFile> in(TFile::Open(inputfile.data(), "READ")),
                           out(TFile::Open(outname.data(), "RECREATE"));
    out->SetCompressionAlgorithm(ROOT::kZLIB);
    out->cd();
    for(auto k :*in->GetListOfKeys())
        k->Write();
}