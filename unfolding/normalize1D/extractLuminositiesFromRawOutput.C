#include "../../meta/stl.C"
#include "../../meta/root.C"

#include "../../struct/DataFileHandler.cxx"
#include "../../struct/LuminosityHandler.cxx"

void extractLuminositiesFromRawOutput(const std::string_view rootfile = "AnalysisResults.root", const std::string_view sysvar = "tc200"){
    DataFileHandler datahandler(rootfile, "FullJets", sysvar);
    LuminosityHandler lumihandler(datahandler);

    std::cout << "Using vertex finding efficiency :" << lumihandler.getVertexFindingEfficiency() << std::endl;
    std::cout << "Using CENTNOTRD correction: " << lumihandler.getCENTNOTRDCorrection() << std::endl;
    std::cout << "Average downscaling for EJ2: " << lumihandler.getEffectiveDownscaleing(LuminosityHandler::TriggerClass::EJ2) << std::endl;
    std::cout << "Using reference luminosity: " << lumihandler.getRefLuminosity() << std::endl;

    std::cout << "INT: " << lumihandler.getRawEvents(LuminosityHandler::TriggerClass::INT7) << " Events, Lint = " << lumihandler.getLuminosity(LuminosityHandler::TriggerClass::INT7, true) << " pb-1" << std::endl;
    std::cout << "EJ2: " << lumihandler.getRawEvents(LuminosityHandler::TriggerClass::EJ2) << " Events, Lint = " << lumihandler.getLuminosity(LuminosityHandler::TriggerClass::EJ2, true) << " pb-1" << std::endl;
    std::cout << "EJ1: " << lumihandler.getRawEvents(LuminosityHandler::TriggerClass::EJ1) << " Events, Lint = " << lumihandler.getLuminosity(LuminosityHandler::TriggerClass::EJ1, true) << " pb-1" << std::endl;

}