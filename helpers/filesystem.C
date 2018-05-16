#ifndef __CLING__
#include <RStringView.h>
#endif

std::string dirname(const std::string_view filename){
  if(filename.find_last_of("/") == std::string::npos) return std::string(""); // No directory found
  return std::string(filename.substr(0, filename.find_last_of("/")));
}