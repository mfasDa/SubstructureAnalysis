#ifndef __ROOT_C__
#define __ROOT_C__

#ifndef __CLING__
#include <vector>
#include <TCollection.h>
#include <TH1.h>
#include <TString.h>
#endif

template<class T>
std::vector<T *> CollectionToSTL(const TCollection *col){
  std::vector<T *> result;
  for(auto o : TRangeDynCast<T>(col)) result.emplace_back(o);
  return result;
}

TH1 *histcopy(const TH1 *inputhist){
  TString histtype = inputhist->IsA()->GetName();
  if(histtype == "TH1F"){
    return new TH1F(*static_cast<const TH1F *>(inputhist));
  } else if(histtype == "TH1D"){
    return new TH1D(*static_cast<const TH1D *>(inputhist));
  } else if(histtype == "TH1C"){
    return new TH1C(*static_cast<const TH1C *>(inputhist));
  } else if(histtype == "TH1S"){
    return new TH1S(*static_cast<const TH1S *>(inputhist));
  } else if(histtype == "TH1I"){
    return new TH1I(*static_cast<const TH1I *>(inputhist));
  } else if(histtype == "TH2F"){
    return new TH2F(*static_cast<const TH2F *>(inputhist));
  } else if(histtype == "TH2D"){
    return new TH2D(*static_cast<const TH2D *>(inputhist));
  } else if(histtype == "TH2C"){
    return new TH2C(*static_cast<const TH2C *>(inputhist));
  } else if(histtype == "TH2I"){
    return new TH2I(*static_cast<const TH2I *>(inputhist));
  } else if(histtype == "TH2S"){
    return new TH2S(*static_cast<const TH2S *>(inputhist));
  } else if(histtype == "TH3F"){
    return new TH3F(*static_cast<const TH3F *>(inputhist));
  } else if(histtype == "TH3D"){
    return new TH3D(*static_cast<const TH3D *>(inputhist));
  } else if(histtype == "TH3S"){
    return new TH3S(*static_cast<const TH3S *>(inputhist));
  } else if(histtype == "TH3C"){
    return new TH3C(*static_cast<const TH3C *>(inputhist));
  } else if(histtype == "TH3I"){
    return new TH3I(*static_cast<const TH3I *>(inputhist));
  } else if(histtype == "TProfile"){
    return new TProfile(*static_cast<const TProfile *>(inputhist));
  }
  return nullptr;
}
#endif