#include "binnings/binningPt1D.C" 
#include "../meta/stl.C"

void printBinning(const std::string_view binningname = "poor") {
    std::vector<double> binningpart, binningdet;
    if(binningname == "poor") {
        binningpart = getJetPtBinningNonLinTruePoor();
        binningdet = getJetPtBinningNonLinSmearPoor();
    }

    std::cout << "Printing det pt bining:" << std::endl; 
    std::cout << "=======================================================" << std::endl;
    bool first = true;
    for(auto ib : binningdet) {
        if(!first) std::cout << ", ";
        else first = false;
        std::cout << ib;
    }
    std::cout << std::endl;
    std::cout << "Printing part pt bining:" << std::endl; 
    std::cout << "=======================================================" << std::endl;
    first = true;
    for(auto ib : binningpart) {
        if(!first) std::cout << ", ";
        else first = false;
        std::cout << ib;
    }
    std::cout << std::endl;
}