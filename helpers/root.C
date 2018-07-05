#ifndef __CLING__
#include <vector>
#include <TCollection.h>
#endif

template<class T>
std::vector<T *> CollectionToSTL(const TCollection *col){
  std::vector<T *> result;
  for(auto o : TRangeDynCast<T>(col)) result.emplace_back(o);
  return result;
}