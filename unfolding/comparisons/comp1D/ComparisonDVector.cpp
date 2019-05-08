std::map<double, TH1 *> readDVector(const std::string_view filename){
    std::map<double, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(const auto k : *reader->GetListOfKeys()){
        double r = double(std::stoi(std::string(k->GetName()).substr(1)))/10.;
        reader->cd(k->GetName());
        gDirectory->cd("reg1");
        auto dvector = static_cast<TH1 *>(gDirectory->Get("dvector_Reg1"));
        dvector->SetDirectory(nullptr);
        result[r] = dvector;
    }
    return result;
}

void ComparisonDVector(const std::string_view filename){
    auto dvecdata = readDVector(filename);

    auto plot = new ROOT6tools::TSavableCanvas("dvectorR", "D-vector for different jet radii", 1200, 800);
    plot->Divide(3,2);

    int icol = 1;
    for(auto [r, spec] : dvecdata) {
        plot->cd(icol++);
        gPad->SetLogy();
        spec->SetTitle(Form("D-Vector, R=%.1f",r));
        spec->SetXTitle("regularization");
        spec->SetYTitle("D-Vector");
        spec->Draw();
    }
    plot->cd();
    plot->Update();
    plot->SaveCanvas(plot->GetName());
}