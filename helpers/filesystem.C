#ifndef __FILESYSTEM_C__
#define __FILESYSTEM_C__

#ifndef __CLING__
#include <RStringView.h>
#include <string>
#endif

std::string dirname(const std::string_view filename){
  if(filename.find_last_of("/") == std::string::npos) return std::string(""); // No directory found
  return std::string(filename.substr(0, filename.find_last_of("/")));
}

std::string basename(std::string_view filename) {
  auto mybasename = filename.substr(filename.find_last_of("/")+1);
  return std::string(mybasename);
}
#endif