#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <string>

#include "sequence/Common.hpp"
#include "sequence/details/StringView.hpp"

namespace sequence {

// An Item represents a sequence entry.
// It can be of the following types :
// - INVALID : the item is not in a valid state.
// - SINGLE  : it's either a single file or directory.
// - INDICED : it's a sequence with a set of numbers attached to it.
// - PACKED  : it's a contiguous sequence going from start to end inclusive.
struct Item {
  enum Type { INVALID, SINGLE, INDICED, PACKED };

  std::string filename;
  Index start = -1, end = -1;
  char padding = -1, step = -1;
  Indices indices;

  Item() = default;
  Item(const Item &other) = default;
  Item(Item &&other) = default;
  Item(CStringView filename);
  Item(CStringView filename, Indices &&indices);
  Item &operator=(const Item &other) = default;
  Item &operator=(Item &&other) = default;

  Type getType() const;

  bool operator<(const Item &other) const;
  bool operator==(const Item &other) const;
};

// For convenience, Items is defined to be a vector of Item
typedef std::vector<Item> Items;
} // namespace sequence
