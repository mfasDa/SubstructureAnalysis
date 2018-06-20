#ifndef __CLING__
#include <vector>
#include <TCollection.h>
#endif

std::vector<TObject *> CollectionToSTL(const TCollection *col){
  std::vector<TObject *> result;
  for(auto o : *col) result.emplace_back(o);
  return result;
}