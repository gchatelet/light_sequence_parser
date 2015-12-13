#include "sequence/Item.hpp"

#include <utility>

#include <gtest/gtest.h>

namespace sequence {
TEST(Items, type) {
  Item item;
  EXPECT_EQ(Item::INVALID, item.getType());
  item.filename = "file";
  EXPECT_EQ(Item::SINGLE, item.getType());
  item.indices.push_back(2);
  EXPECT_EQ(Item::INDICED, item.getType());
  item.indices.clear();
  EXPECT_EQ(Item::SINGLE, item.getType());
  item.step = 0;
  EXPECT_EQ(Item::PACKED, item.getType());
}

TEST(Items, defaultCTor) {
  Item item;
  EXPECT_EQ(item.start, -1);
  EXPECT_EQ(item.end, -1);
  EXPECT_EQ(item.padding, -1);
  EXPECT_EQ(item.step, -1);
}

TEST(Items, moveCTor) {
  Item origin;
  origin.start = 1;
  origin.end = 2;
  origin.padding = 3;
  origin.step = 4;
  origin.filename = "filename";
  origin.indices = {1, 2, 3, 4};
  const Item moved(std::move(origin));
  EXPECT_EQ(moved.start, 1);
  EXPECT_EQ(moved.end, 2);
  EXPECT_EQ(moved.padding, 3);
  EXPECT_EQ(moved.step, 4);
  EXPECT_EQ(moved.filename, "filename");
  EXPECT_EQ(moved.indices, Indices({1, 2, 3, 4}));
  EXPECT_TRUE(origin.filename.empty());
  EXPECT_TRUE(origin.indices.empty());
}

} // namespace sequence
