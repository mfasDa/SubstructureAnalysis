#ifndef __CLING__
#include <iostream>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <RStringView.h>
#include <TFile.h>
#include <TH1.h>
#include <TKey.h>
#include <TMath.h>
#endif

#include "../../helpers/msl.C"

void Renormalize(TH1 *histtonorm, double jetradius, int neventsMB) {
    const double kSizeEmcalPhi = 1.74,
                 kSizeEmcalEta = 1.4;
    double acceptance = (kSizeEmcalPhi - 2 * jetradius) * (kSizeEmcalEta - jetradius) / (TMath::TwoPi());
    double crosssection = 57.8;
    for(auto b : ROOT::TSeqI(0, histtonorm->GetXaxis()->GetNbins())) {
        double norm = crosssection/(histtonorm->GetXaxis()->GetBinWidth(b+1) * neventsMB * acceptance);
        histtonorm->SetBinContent(b+1, histtonorm->GetBinContent(b+1) * norm);
        histtonorm->SetBinError(b+1, histtonorm->GetBinError(b+1) * norm);
    }
}

int getRegularization(const std::string_view dirname){
    std::string regstring = std::string(dirname);
    if(contains(regstring, "iteration")) regstring.erase(regstring.find("iteration"), strlen("iteration"));
    else if(contains(regstring, "regularization")) regstring.erase(regstring.find("regularization", strlen("regularization")));
    return std::stoi(regstring);
}

std::map<int, TH1 *> getUnfolded(const std::string_view filename) {
    std::map<int, TH1 *> result;
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    for(auto mykey : TRangeDynCast<TKey>(reader->GetListOfKeys())){
        if(!contains(mykey->GetName(), "iteration") && !contains(mykey->GetName(), "regularization")) continue;
        std::cout << "Reading " << mykey->GetName() << std::endl;
        reader->cd(mykey->GetName());
        auto histos = CollectionToSTL<TObject>(gDirectory->GetListOfKeys());
        auto histkey = static_cast<TKey *>(*std::find_if(histos.begin(), histos.end(), [](TObject *o) { return contains(o->GetName(), "unfolded") && !contains(o->GetName(), "Closure"); }));
        auto spectrum = histkey->ReadObject<TH1>();
        auto regval = getRegularization(mykey->GetName());
        spectrum->SetDirectory(nullptr);
        spectrum->SetName(Form("unfolded_r%d", regval));
        std::cout << "Auto-detected regularization parameter " << regval << std::endl;
        result[regval] = spectrum;
    }
    return result;
}

double readEventCounter(const std::string_view filename, const std::string_view tag){
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::stringstream jetdir;
    jetdir << "JetSubstructure_" << tag.data();
    std::cout << "Reading normalization from jet directory " << jetdir.str() << std::endl;
    reader->cd(jetdir.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto lumi = static_cast<TH1 *>(histlist->FindObject("hEventCounter"));
    return lumi->GetBinContent(1);
}

double readLuminosityCounter(const std::string_view filename, const std::string_view tag) {
    std::unique_ptr<TFile> reader(TFile::Open(filename.data(), "READ"));
    std::stringstream jetdir;
    jetdir << "JetSubstructure_" << tag.data();
    std::cout << "Reading normalization from jet directory " << jetdir.str() << std::endl;
    reader->cd(jetdir.str().data());
    auto histlist = static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>();
    auto lumi = static_cast<TH1 *>(histlist->FindObject("hLumiMonitor"));
    auto jd = getJetType(tag);
    double nevents = lumi->GetBinContent(lumi->GetXaxis()->FindBin("CENT"));
    if(jd.fTrigger == "EJ1") {
        // apply livetime correction
        auto hclasses = static_cast<TH1 *>(histlist->FindObject("fHistTriggerClasses"));
        double ncent = hclasses->GetBinContent(hclasses->GetXaxis()->FindBin("CEMC7EJ1_B_NOPF_CENT")),
               ncentnotrd = hclasses->GetBinContent(hclasses->GetXaxis()->FindBin("CEMC7EJ1_B_NOPF_CENTNOTRD"));
        auto livetimecorrection = ncentnotrd/ncent;
        std::cout << "Applying livetime correction factor " << livetimecorrection << std::endl;
        nevents *= livetimecorrection;
    }
    return nevents;
}

std::string getFileTag1D(const std::string_view unfoldedfile) {
    std::string tag = basename(unfoldedfile);
    tag.erase(tag.find(".root"), 5);
    tag.erase(0, tag.find_first_of("_")+1);
    return tag;
}

std::string getUnfoldingMethod(const std::string_view unfoldedfile) {
    std::string tag = basename(unfoldedfile);
    tag.erase(tag.find_first_of("_"));
    tag.erase(tag.find("unfolded"), strlen("unfolded"));
    return tag;
}

void normalize1D(const std::string_view unfoldedfile, const std::string_view eventcountfile){
    auto tag = getFileTag1D(unfoldedfile);
    auto jd = getJetType(tag);

    // Triggers to be added later
    auto norm = contains(jd.fTrigger, "INT7") ? readEventCounter(eventcountfile, tag) : readLuminosityCounter(eventcountfile, tag);
    auto unfolded = getUnfolded(unfoldedfile);
    std::map<int, TH1 *> normalized;
    for(auto h : unfolded) {
        std::cout << "Normalizing iteration " << h.first << std::endl;
        auto specnormalized = histcopy(h.second);
        specnormalized->SetDirectory(nullptr);
        specnormalized->SetName(Form("normalized_reg%d", h.first));
        Renormalize(specnormalized, jd.fJetRadius, norm);
        normalized[h.first] = specnormalized;
    }

    // Write output
    auto dirnameUnf = dirname(unfoldedfile);
    std::stringstream outfilename;
    if(dirnameUnf.length()) outfilename << dirnameUnf << "/";
    outfilename << "normalized" << getUnfoldingMethod(unfoldedfile) << "_" << tag << ".root";
    std::unique_ptr<TFile> writer(TFile::Open(outfilename.str().data(), "RECREATE"));
    writer->cd();
    norm->Write();
    for(auto iter : unfolded) {
        std::stringstream regdir;
        regdir << "regularization" << iter.first;
        writer->mkdir(regdir.str().data());
        writer->cd(regdir.str().data());
        iter.second->Write();
        normalized[iter.first]->Write();
    }
}