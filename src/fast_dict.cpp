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
//#include "../../hashes/clhash/include/clhash.h"
//#include "../../hashes/clhash/src/clhash.c"
//#include "../../hashes/xxhash_cpp/include/xxhash.hpp"

class StrokaHasher;

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
    static const int p = 31;
    //static const int m = 1e9 + 9;
    size_t hash_value = 0;
    for (int i = 0; i < s.size_; ++i) {
      hash_value = (hash_value * p + (s.s_[i] - 'a' + 1));// % m;
    }
    return hash_value;
    //return s.hash_;
   // return MurmurHash64A(s.s_, static_cast<uint64_t>(s.size_), 0);
  }
};

class FastHasher {
public:
  using hash_policy = ska::power_of_two_hash_policy;

  static void fill() {
    generate_table(table_);
  }

  static void generate_table(uint32_t (&table)[256]) {
    uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t c = i;
      for (size_t j = 0; j < 8; j++) {
        if (c & 1) {
          c = polynomial ^ (c >> 1);
        } else {
          c >>= 1;
        }
      }
      table[i] = c;
    }
  }

  static uint32_t update(const uint32_t (&table)[256], uint32_t initial,
                         const void *buf, size_t len) {
    uint32_t c = initial ^ 0xFFFFFFFF;
    const uint8_t *u = static_cast<const uint8_t *>(buf);
    for (size_t i = 0; i < len; ++i) {
      c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
  }

  inline uint32_t hash(const std::string& s) {
    return update(table_, 42, s.data(), s.size());
  }

  inline size_t operator()(const std::string& s) {
    return hash(s);
  }

  static uint32_t table_[256];
};

uint32_t FastHasher::table_[256];

class power_two_hasher : public std::hash<std::string> {
public:
  using hash_policy = ska::power_of_two_hash_policy;

  //inline size_t operator()(const std::string& s) const noexcept {



//    constexpr static const int p = 31;
//    //static const int m = 1e9 + 9;
//    size_t hash_value = 0;
//    for (char c: s) {
//      hash_value = (hash_value * p + (c - 'a' + 1));// % m;
//    }
//    return hash_value;

//    static clhasher h(UINT64_C(0x23a23cf5033c3c81),UINT64_C(0xb3816f6a2c68e530));
//    return h(s);
    //return s.hash_;
   //  return MurmurHash64A(s.c_str(), static_cast<uint64_t>(s.size()), 0);
    // return xxh::xxhash<64>(s);
    //return fastHasher.hash(s);
  //}
};

struct Counter {
  uint32_t count = 0;
};

//using stringMap = ska::flat_hash_map<Stroka, Counter, StrokaHasher>;
using stringMap = ska::flat_hash_map<std::string, Counter, FastHasher>;

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

  FastHasher::fill();

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
        uint8_t lc = check + 'a';
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