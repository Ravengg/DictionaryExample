#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <cassert>
#include <limits>

#include "FlatHashMap.hpp"

class StrokaHasher;

inline uint32_t MurmurHash64A(const void *key, uint32_t len, unsigned int seed)
{
  //return ((uint32_t*)key)[0];
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const auto *data = (const uint64_t *)key;
  const uint64_t *end = data + (len / 8);

  while (data != end)
  {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const auto *data2 = (const unsigned char *)data;

  switch (len & 7)
  {
  case 7: h ^= uint64_t(data2[6]) << 48; [[fallthrough]];
  case 6: h ^= uint64_t(data2[5]) << 40; [[fallthrough]];
  case 5: h ^= uint64_t(data2[4]) << 32; [[fallthrough]];
  case 4: h ^= uint64_t(data2[3]) << 24; [[fallthrough]];
  case 3: h ^= uint64_t(data2[2]) << 16; [[fallthrough]];
  case 2: h ^= uint64_t(data2[1]) << 8; [[fallthrough]];
  case 1: h ^= uint64_t(data2[0]); h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}


class Stroka { // yandex style
  friend StrokaHasher;
public:
  explicit Stroka(const std::string& s) noexcept :
    s_(s.c_str()),
    size_(s.size())//,
   // hash_(0)
  {
//    for (char c: s) {
//      hash_ = c + hash_ * 29;
//    }
  }

  Stroka(const char* s, size_t size) noexcept :
    s_(s),
    size_(size)
  {}

  bool operator==(const Stroka& r) const noexcept {
    return size_ == r.size_ && !strcmp(s_, r.s_);
  }

  std::string toString() const noexcept {
    return std::string(s_, size_);
  }

private:
  const char* s_;
  size_t size_;
//  size_t hash_;
};

class StrokaHasher {
public:
  using hash_policy = ska::power_of_two_hash_policy;

  inline size_t operator()(const Stroka& s) const noexcept {
    //return s.hash_;
    return MurmurHash64A(s.s_, static_cast<uint64_t>(s.size_), 0);
  }
};

class power_two_hasher : public std::hash<std::string> {
public:
  using hash_policy = ska::power_of_two_hash_policy;
};

struct Counter {
  uint32_t count = 0;
};

//using stringMap = ska::flat_hash_map<Stroka, Counter, StrokaHasher>;
using stringMap = ska::flat_hash_map<std::string, Counter, power_two_hasher>;

//inline void putState(stringMap& dict,
//                     const char* state, int state_size, std::vector<char*>& stringKeeper) noexcept {
//  if (!state_size) {
//    return;
//  }
//  auto iter = dict.find(Stroka(state, state_size));
//  if (iter == dict.end()) {
//    char* new_str = new char[state_size + 1];
//    strcpy(new_str, state);
//    stringKeeper.push_back(new_str);
//    dict.insert(std::make_pair(Stroka(new_str, state_size), 1)); //compute hash twice, skip now
//  } else {
//    ++iter->second;
//  }
//}

inline void putState(stringMap& dict, std::string& state) noexcept {
  if (state.empty()) {
    return;
  }
  ++dict[state].count;
  state.clear();
}

inline bool cmp(const std::pair<uint32_t, std::string>& l, const std::pair<uint32_t, std::string>& r) {
  return l.first > r.first || l.first == r.first && l.second < r.second;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    throw std::runtime_error("should be 2 args: in and out files");
  }
  FILE* in = fopen(argv[1], "rb");
  if (in == nullptr) {
    throw std::runtime_error("can't open input file");
  }
  FILE* out = fopen(argv[2], "w");
  if (out == nullptr) {
    throw std::runtime_error("can't open output file");
  }

//  char state[1024];
//  size_t state_size = 0;

  std::string state;

  char curByte;

  stringMap dict;
  dict.reserve(1024);
  dict.max_load_factor(0.3);

//  std::vector<char*> stringKeeper;
//  stringKeeper.reserve(1024);
  size_t bufSize = 1024;
  char buf[bufSize];
  while(true) {
    int size = fread(buf, sizeof(char), bufSize, in);
    for (int i = 0; i < size; ++i) {
      char curByte = buf[i];
      //assert(state_size < 1023 && "long strings aren't allowed");
      uint8_t c = curByte - 'A';
      uint8_t check = (c & (uint8_t)(255 - 32));
      if (check < 26) {
        uint8_t lc = (c & (uint8_t)(31)) + 'a';
//        state[state_size] = lc;
//        ++state_size;
        state.push_back(lc);
      } else {
       // state[state_size] = '\0';
        putState(dict, state);//, state_size, stringKeeper);
        //state_size = 0;
      }
    }
    if (size != bufSize) {
      break;
    }
  }
  putState(dict, state);//, state_size, stringKeeper);

  std::vector<std::pair<uint32_t, std::string>> v;
  v.reserve(dict.size());

  for(auto&& elem: dict) {
    v.push_back(std::make_pair(elem.second.count, std::move(elem.first)));
  }
//  for(auto elem: stringKeeper) {
//    delete[] elem;
//  }

  std::sort(v.begin(), v.end(), cmp);

  for(auto& elem: v) {
    fprintf(out, "%u %s\n", elem.first, elem.second.c_str());
  }

  fclose(in);
  fclose(out);
}