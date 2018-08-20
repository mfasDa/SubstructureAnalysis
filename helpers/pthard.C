#ifndef __CLING__
#include <map>
#endif

bool IsOutlier(double ptjetsim, int pthardbin, double outliercut = 2.) {
  std::map<int, std::pair<double, double>> pthardbins = {
    {0, {0,5}}, {1, {5, 7}}, {2, {7, 9}}, {3, {9, 12}}, {4, {12, 16}}, {5, {16, 21}}, {6, {21, 28}}, 
    {7, {28, 36}}, {8, {36, 45}}, {9, {45, 57}}, {10, {57, 70}}, {11, {70, 85}}, {12, {85, 99}}, {13, {99, 115}}, 
    {14, {115, 132}}, {15, {132, 150}}, {16, {150, 169}}, {17, {150, 190}}, {18, {190, 212}}, {19, {212, 235}}, {20, {235, 1000}} 
  };

  if(pthardbin > 20) return false;
  return ptjetsim > outliercut * pthardbins[pthardbin].second;
}