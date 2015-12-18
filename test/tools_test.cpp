#include "sequence/Tools.hpp"

#include <iostream>
#include <utility>

#include <gtest/gtest.h>

void PrintTo(const CStringView &view, ::std::ostream *os) {
  os->write(view.ptr(), view.size());
}
typedef std::pair<CStringView, CStringView> Pair;
void PrintTo(const Pair &pair, ::std::ostream *os) {
  *os << "(\"";
  PrintTo(pair.first, os);
  *os << "\",\"";
  PrintTo(pair.second, os);
  *os << "\")";
}

namespace sequence {

TEST(Tools, createPattern) {
  EXPECT_EQ(createPattern("a", "b", 1), "a#b");
  EXPECT_EQ(createPattern("a", "b", 0), "a#b");
  EXPECT_EQ(createPattern("a", "b", 3), "a###b");
  EXPECT_EQ(createPattern("", "b", 1), "#b");
  EXPECT_EQ(createPattern("a", "", 1), "a#");
  EXPECT_EQ(createPattern("", "", 10), "##########");
  EXPECT_THROW(createPattern("", "", 11), std::domain_error);
}

TEST(Tools, getPrefixAndSuffix) {
  EXPECT_EQ(getPrefixAndSuffix("a#b"), Pair("a", "b"));
  EXPECT_EQ(getPrefixAndSuffix("a#"), Pair("a", ""));
  EXPECT_EQ(getPrefixAndSuffix("#b"), Pair("", "b"));
  EXPECT_EQ(getPrefixAndSuffix("#"), Pair("", ""));
  EXPECT_EQ(getPrefixAndSuffix("a##b"), Pair("a", "b"));
  EXPECT_EQ(getPrefixAndSuffix("a##########b"), Pair("a", "b"));
  EXPECT_THROW(getPrefixAndSuffix("a###########b"), std::invalid_argument);
  EXPECT_THROW(getPrefixAndSuffix("a#c#b"), std::invalid_argument);
}

TEST(Tools, createFile) {
  // a file that looks like a pattern is valid
  EXPECT_EQ(createSingleFile("file#.jpg").getType(), Item::SINGLE);

  const Item item = createSingleFile("file.jpg");
  EXPECT_EQ(item.getType(), Item::SINGLE);
  EXPECT_EQ(item.filename, "file.jpg");
}

TEST(Tools, createEmptySequence) {
  // empty prefix, suffix is ok
  const Item item = createSequence("#", 0, 0);
  EXPECT_EQ(item.getType(), Item::PACKED);
  EXPECT_EQ(item.filename, "#");
  EXPECT_EQ(item.padding, 1);
  EXPECT_EQ(item.start, 0);
  EXPECT_EQ(item.end, 0);
}

TEST(Tools, createSequence) {
  const Item item = createSequence("file-###.png", 10, 20);
  EXPECT_EQ(item.getType(), Item::PACKED);
  EXPECT_EQ(item.filename, "file-###.png");
  EXPECT_EQ(item.padding, 3);
  EXPECT_EQ(item.start, 10);
  EXPECT_EQ(item.end, 20);
}

TEST(Tools, createIndicedSequence) {
  const Item item = createSequence("file#.png", {8, 10, 16});
  EXPECT_EQ(item.getType(), Item::INDICED);
  EXPECT_EQ(item.filename, "file#.png");
  EXPECT_EQ(item.indices, Indices({8, 10, 16}));
}

TEST(Tools, createInvalidSequence) {
  // invalid if end < start
  EXPECT_EQ(createSequence("#", 10, 0).getType(), Item::INVALID);
  // invalid if step is 0
  EXPECT_EQ(createSequence("#", 0, 0, 0).getType(), Item::INVALID);
}

TEST(Tools, createSequenceFilename) {
  EXPECT_EQ(createSequence("file-#.png", 0, 0).filename, "file-#.png");
  EXPECT_EQ(createSequence("file-###.png", 0, 0).filename, "file-###.png");
  EXPECT_EQ(createSequence("file-#.tif", 0, 0).filename, "file-#.tif");
  EXPECT_EQ(createSequence("file-#-.cr2", 0, 0).filename, "file-#-.cr2");
}

TEST(Tools, matcherItem) {
  EXPECT_EQ(details::getMatcherString("@"), "#+");
  EXPECT_EQ(details::getMatcherString("file###.jpg"), "file###\\.jpg");
  EXPECT_EQ(details::getMatcherString("*#.jpg"), ".*#+\\.jpg");
  EXPECT_THROW(details::getMatcherString(""), std::invalid_argument);
  EXPECT_THROW(details::getMatcherString("missing_padding_character"),
               std::invalid_argument);
  EXPECT_EQ(details::getMatcherString("file-#.png"), "file-#+\\.png");
}

TEST(Tools, match) {

  EXPECT_TRUE(
      match(getMatcher("file-#.png"), createSequence("file-#.png", 0, 0)));
  EXPECT_TRUE(
      match(getMatcher("file-@.png"), createSequence("file-#.png", 0, 0)));
  EXPECT_TRUE(
      match(getMatcher("file-@.png"), createSequence("file-###.png", 0, 0)));
  EXPECT_TRUE(match(getMatcher("file-#*"), createSequence("file-#.tif", 0, 0)));
  EXPECT_TRUE(match(getMatcher("*-#-*"), createSequence("file-#-.cr2", 0, 0)));
}

TEST(Tools, filter) {
  Items items = {createSequence("file-#.png", 1, 2),
                 createSequence("file-#.jpg", 1, 2)};
  const std::regex matcher = getMatcher("file-@.png");
  auto predicate = [&](const Item &item) -> bool {
    return !match(matcher, item);
  };
  items.erase(std::remove_if(items.begin(), items.end(), predicate),
              items.end());
  EXPECT_EQ(items.size(), 1);
}

} // namespace sequence
