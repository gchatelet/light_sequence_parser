#include <sequence/details/Hash.hpp>

#include "MurmurHash3.h"

uint32_t hash(CStringView view) {
  uint32_t hash;
  MurmurHash3_x86_32(view.ptr(), view.size(), 0 /*seed*/, &hash);
  return hash;
}

uint32_t hash(uint32_t value) {
  return hash(
      CStringView(reinterpret_cast<const char *>(&value), sizeof(value)));
}