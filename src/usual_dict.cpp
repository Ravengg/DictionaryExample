#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

inline void putState(std::unordered_map<std::string, size_t>& dict, std::string& state) {
  if (!state.empty()) {
    auto iter = dict.find(state);
    if (iter == dict.end()) {
      dict.insert(std::make_pair(state, 1));
    } else {
      ++iter->second;
    }
    state.clear();
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    throw std::runtime_error("should be 2 args: in and out files");
  }
  std::ifstream in(argv[1], std::ios::binary);
  if (!in.is_open()) {
    throw std::runtime_error("can't open input file");
  }
  std::ofstream out(argv[2]);
  if (!out.is_open()) {
    throw std::runtime_error("can't open output file");
  }

  std::string state;
  char curByte;

  std::unordered_map<std::string, size_t> dict;
  while(in.get(curByte)) {
    if ('A' <= curByte && curByte <= 'Z') {
      state.push_back(curByte - ('A' - 'a'));
    } else if ('a' <= curByte && curByte <= 'z') {
      state.push_back(curByte);
    } else {
      putState(dict, state);
    }
  }
  putState(dict, state);

  std::vector<std::pair<size_t, std::string>> v;
  v.reserve(dict.size());

  for(auto&& elem: dict) {
    v.push_back(std::make_pair(elem.second, std::move(elem.first)));
  }
  std::sort(v.begin(), v.end(),
            [](auto a, auto b) {
              return a.first > b.first || a.first == b.first && a.second < b.second;
            });

  for(auto& elem: v) {
    out << elem.first << ' ' << elem.second << std::endl;
  }

  in.close();
  out.close();
}