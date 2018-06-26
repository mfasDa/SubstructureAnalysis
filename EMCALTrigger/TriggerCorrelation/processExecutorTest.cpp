#ifndef __CLING__
#include <algorithm>
#include <iostream>
#include <vector>
#include <tuple>
#include <ROOT/TProcessExecutor.hxx>
#endif

void processExecutorTest(){
  ROOT::TProcessExecutor test(10);
  auto result = test.MapReduce([](int iv) { 
    std::vector<std::tuple<int, int, int, int>> tmp; 
    for(int i = 0; i < 10; i++) tmp.push_back(std::make_tuple(iv*10+i, 100 + iv*10+i, 200 + iv*10+i, 300 + iv*10+i));
    return tmp;
    },
    ROOT::TSeqI(0, 10),
    [](const std::vector<std::vector<std::tuple<int, int, int, int>>> &data){
      std::vector<std::tuple<int, int, int, int>> result;
      for(auto d : data){
        for(auto e : d) result.emplace_back(e);
      }
      std::sort(result.begin(), result.end(), [](const std::tuple<int, int, int, int> &first, const std::tuple<int, int, int, int> &second) { return std::get<0>(first) < std::get<0>(second);} );
      return result;
    }
  ); 

  for(auto i : result) std::cout << std::get<0>(i) << ", " << std::get<1>(i) << ", " << std::get<2>(i) << ", " << std::get<3>(i) << std::endl;
}