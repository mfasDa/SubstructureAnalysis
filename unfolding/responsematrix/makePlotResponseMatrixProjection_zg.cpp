#ifndef __CLING__
#include <RStringView.h>
#endif

#include "makePlotResponseMatrixProjectionPt.cpp"
#include "makePlotResponseMatrixProjectionShape.cpp"

void makePlotResponseMatrixProjection_zg(const std::string_view filename){
    makePlotResponseMatrixProjectionPt(filename, "zg");
    makePlotResponseMatrixProjectionShape(filename, "zg");
}