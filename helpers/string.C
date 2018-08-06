#ifndef __STRING_C__
#define __STRING_C__

#ifndef __CLING__
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#endif

std::vector<std::string> tokenize(const std::string &strtotok, char token = '\n'){
  std::stringstream tokenizer(strtotok);
  std::vector<std::string> result;
  std::string tmp;
  while(std::getline(tokenizer, tmp, token)) result.emplace_back(tmp);
  return result;
}

std::string trim(const std::string &s)
{
   auto wsfront=std::find_if_not(s.begin(),s.end(),[](int c){return std::isspace(c);});
   auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](int c){return std::isspace(c);}).base();
   return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
}

bool is_number(const std::string& s)
{
  auto trimmed = trim(s);
  return !trimmed.empty() && std::find_if(trimmed.begin(), trimmed.end(), [](char c) { return !std::isdigit(c); }) == trimmed.end();
}

bool contains(const std::string_view base, const std::string_view tokens){
  return base.find(tokens) != std::string::npos;
}
#endif