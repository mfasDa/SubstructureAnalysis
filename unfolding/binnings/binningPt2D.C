#ifndef __BINNINGPT2D_H__
#define __BINNINGPT2D_H__

#include "../../meta/stl.C"

std::vector<double> getDetPtBinning(const std::string_view variation){
    if(variation == "default") {
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 60., 80., 100., 120., 140., 160., 180., 200.};
    } else if(variation == "truncationLowLoose") {
        return {5., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 60., 80., 100., 120., 140., 160., 180., 200.};
    } else if (variation == "truncationLowStrong") {
        return {7., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 60., 80., 100., 120., 140., 160., 180., 200.};
    } else if (variation == "truncationHighLoose") {
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 60., 80., 100., 120., 140., 160., 180., 210.};
    } else if (variation == "truncationHighStrong") {
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 60., 80., 100., 120., 140., 160., 180., 190.};
    } else if (variation == "binning1") {    // shift low 1 GeV
        return {6., 7., 9., 11., 13., 15., 17., 19., 24., 29., 34., 39., 49., 59., 79., 99., 119., 139., 159., 179., 200.};
    } else if (variation == "binning2") {    // shift high 1 GeV
        return {6., 9., 11., 13., 15., 17., 19., 21., 26., 31., 36., 41., 51., 61., 81., 101., 121., 141., 161., 181., 200.};
    } else if (variation == "binning3") {    // squeeze low pt
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 29., 34., 39., 48., 57., 72., 90., 110., 131., 153., 175., 200.};
    } else if (variation == "binning4") {    // squeeze high pt
        return {6., 9., 12., 15., 19., 25., 31., 37., 44., 52., 61., 71., 82., 94., 107., 121., 136., 150., 166., 183., 200.};
    } else if (variation == "ej2low") {
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 55., 80., 100., 120., 140., 160., 180., 200.};
    } else if (variation == "ej2high") {
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 65., 80., 100., 120., 140., 160., 180., 200.};
    } else if (variation == "ej1low") {
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 60., 80., 95., 120., 140., 160., 180., 200.};
    } else if (variation == "ej1high") {
        return {6., 8., 10., 12., 14., 16., 18., 20., 25., 30., 35., 40., 50., 60., 80., 105., 120., 140., 160., 180., 200.};
    } else {
        return std::vector<double>();
    }
}

#endif
