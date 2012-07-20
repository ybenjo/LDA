#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <time.h>

typedef std::pair<int, int> key;

struct myeq : std::binary_function<std::pair<int, int>, std::pair<int, int>, bool>{
  bool operator() (const std::pair<int, int> & x, const std::pair<int, int> & y) const{
    return x.first == y.first && x.second == y.second;
  }
};

struct myhash : std::unary_function<std::pair<int, int>, size_t>{
private:
  const std::hash<int> h_int;
public:
  myhash() : h_int() {}
  size_t operator()(const std::pair<int, int> & p) const{
    size_t seed = h_int(p.first);
    return h_int(p.second) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }
};

std::vector<std::string> split_string(std::string s, std::string c){
  std::vector<std::string> ret;
  for(int i = 0, n = 0; i <= s.length(); i = n + 1){
    n = s.find_first_of(c, i);
    if(n == std::string::npos) n = s.length();
    std::string tmp = s.substr(i, n-i);
    ret.push_back(tmp);
  }
  return ret;
}

double uniform_rand(){
  return (double)rand() * (1.0 / (RAND_MAX + 1.0));
}
