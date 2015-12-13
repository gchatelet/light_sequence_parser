#include "sequence/details/Utils.hpp"

#include <gtest/gtest.h>

namespace sequence {
namespace details {

TEST(IndexParser, empty) {
  IndexParser parser("");
  EXPECT_EQ(parser.index, 0);
  EXPECT_FALSE(parser.overflowed);
}

TEST(IndexParser, simple) {
  IndexParser parser("12");
  EXPECT_EQ(parser.index, 12);
  EXPECT_FALSE(parser.overflowed);
}

TEST(IndexParser, not_overflowed) {
  // 4,294,967,295
  IndexParser parser("4294967295");
  EXPECT_EQ(parser.index, 4294967295);
  EXPECT_FALSE(parser.overflowed);
}

TEST(IndexParser, overflowed) {
  // 4,294,967,295 + 1
  IndexParser parser("4294967296");
  EXPECT_TRUE(parser.overflowed);
}

TEST(integerRanges, empty) {
  std::string str;
  const std::vector<StringView> output(integerRanges(str));
  EXPECT_TRUE(output.empty());
}

TEST(integerRanges, no_match) {
  std::string str("abc");
  const std::vector<StringView> output(integerRanges(str));
  EXPECT_TRUE(output.empty());
}

TEST(integerRanges, full) {
  std::string str("123");
  const std::vector<StringView> output(integerRanges(str));
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "123");
}

TEST(integerRanges, prefix) {
  std::string str("abc123");
  const std::vector<StringView> output(integerRanges(str));
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "123");
}

TEST(integerRanges, suffix) {
  std::string str("123abc");
  const std::vector<StringView> output(integerRanges(str));
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "123");
}

TEST(integerRanges, prefix_suffix) {
  std::string str("ab123abc");
  const std::vector<StringView> output(integerRanges(str));
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "123");
}

TEST(integerRanges, many) {
  std::string str("a1b2");
  const std::vector<StringView> output(integerRanges(str));
  ASSERT_EQ(output.size(), 2);
  EXPECT_EQ(output[0], "1");
  EXPECT_EQ(output[1], "2");
}

TEST(getText, none) {
  EXPECT_EQ(getText(""), std::vector<CStringView>());
  EXPECT_EQ(getText("####"), std::vector<CStringView>());
}

TEST(getText, onlyText) {
  EXPECT_EQ(getText("abc"), std::vector<CStringView>({"abc"}));
}

TEST(getText, pattern) {
  EXPECT_EQ(getText("a#b#c"), std::vector<CStringView>({"a", "b", "c"}));
}

TEST(bake, simple) {
  std::string str("/path/to/file##_###.cr#");
  bake(str, 1, 12);
  EXPECT_EQ(str, "/path/to/file##_012.cr#");
}

TEST(extractFileNumbersAndNormalize, empty) {
  Indices indices;
  std::string output;
  extractFileIndicesAndNormalize(output, indices);
  EXPECT_TRUE(output.empty());
  EXPECT_TRUE(indices.empty());
}

TEST(extractFileNumbersAndNormalize, no_number) {
  Indices indices;
  std::string output("no_numbers");
  extractFileIndicesAndNormalize(output, indices);
  EXPECT_EQ(output, "no_numbers");
  EXPECT_TRUE(indices.empty());
}

TEST(extractFileNumbersAndNormalize, many_numbers) {
  Indices indices;
  std::string output("numbers1_23_5");
  extractFileIndicesAndNormalize(output, indices);
  EXPECT_EQ(output, "numbers#_##_#");
  EXPECT_EQ(indices, Indices({1, 23, 5}));
}

TEST(extractFileNumbersAndNormalize, path_numbers_are_untouched) {
  Indices indices;
  std::string output("path1/numbers1");
  extractFileIndicesAndNormalize(output, indices);
  EXPECT_EQ(output, "path1/numbers#");
  EXPECT_EQ(indices, Indices({1}));
}

TEST(extractFileNumbersAndNormalize, toobig_is_untouched) {
  Indices indices;
  std::string output("numbers4294967296.jpg");
  extractFileIndicesAndNormalize(output, indices);
  EXPECT_EQ(output, "numbers4294967296.jpg");
  EXPECT_TRUE(indices.empty());
}

TEST(extractFileNumbersAndNormalize, toobig_is_untouched_but_others) {
  Indices indices;
  std::string output("numbers4294967296_12.jpg");
  extractFileIndicesAndNormalize(output, indices);
  EXPECT_EQ(output, "numbers4294967296_##.jpg");
  EXPECT_EQ(indices, Indices({12}));
}

TEST(extractFileNumbersAndNormalize, extension_is_untouched) {
  Indices indices;
  std::string output("numbers_12.jpg2k");
  extractFileIndicesAndNormalize(output, indices);
  EXPECT_EQ(output, "numbers_##.jpg2k");
  EXPECT_EQ(indices, Indices({12}));
}

TEST(estimateDistinctValue, Empty) {
  Indices indices;
  EXPECT_EQ(estimateDistinctIndices(indices), 0);
}

TEST(estimateDistinctValue, One) {
  Indices indices({10});
  EXPECT_EQ(estimateDistinctIndices(indices), 1);
}

TEST(estimateDistinctValue, Two) {
  Indices indices({10, 11});
  EXPECT_EQ(estimateDistinctIndices(indices), 2);
}

TEST(estimateDistinctValue, Many) {
  Indices indices;
  indices.reserve(1000);
  for (int i = 0; i < 1000; ++i)
    indices.push_back(i);
  EXPECT_EQ(estimateDistinctIndices(indices), 1000);
}

TEST(estimateDistinctValue, Estimated) {
  Indices indices;
  indices.reserve(10000);
  for (int i = 0; i < 10000; ++i)
    indices.push_back(i);
  const auto estimated = estimateDistinctIndices(indices);
  EXPECT_GE(estimated, 9500);
  EXPECT_LE(estimated, 10000);
}

TEST(Bucket, splitConstant) {
  Bucket a;
  a.pattern = "/path/file###.cr#";
  a.columns.push_back(Indices({1, 2, 3}));
  a.columns.push_back(Indices({2, 2, 2}));
  Buckets results;
  a.split(1, [&results](Bucket v) { results.push_back(std::move(v)); });
  ASSERT_EQ(results.size(), 1);
  const auto &result = results[0];
  EXPECT_EQ(result.pattern, "/path/file###.cr2");
  ASSERT_EQ(result.columns.size(), 1);
  EXPECT_EQ(result.columns[0], Indices({1, 2, 3}));
}

TEST(Bucket, splitLinear) {
  Bucket a;
  a.pattern = "/path/file###.cr#";
  a.columns.push_back(Indices({1, 2, 3}));
  a.columns.push_back(Indices({2, 2, 2}));
  Buckets results;
  a.split(0, [&results](Bucket v) { results.push_back(std::move(v)); });
  ASSERT_EQ(results.size(), 3);
  for (const auto &result : results) {
    EXPECT_TRUE(result.pattern == "/path/file001.cr#" ||
                result.pattern == "/path/file002.cr#" ||
                result.pattern == "/path/file003.cr#");
    ASSERT_EQ(result.columns.size(), 1);
    EXPECT_EQ(result.columns[0], Indices({2}));
  }
}

TEST(SplitBucket, step) {
  EXPECT_EQ(getStep({1, 2, 3}), 1);
  EXPECT_EQ(getStep({2, 4, 6, 22, 24}), 2);
  EXPECT_EQ(getStep({0, 200, 400}), -1);
  EXPECT_EQ(getStep({0, 2}), 2);
  EXPECT_EQ(getStep({0}), -1);
}

SplitBucket make(CStringView pattern, std::initializer_list<Index> indices) {
  Bucket bucket(pattern);
  bucket.columns.push_back(indices);
  return {std::move(bucket)};
}

SplitBucket makeAndPack(CStringView pattern,
                        std::initializer_list<Index> indices) {
  auto bucket = make(pattern, indices);
  bucket.pack();
  return bucket;
}

TEST(SplitBucket, check) {
  const auto a = make("a##b", {3, 2});
  EXPECT_EQ(a.pattern, "a##b");
  EXPECT_EQ(a.sortedIndices, Indices({2, 3}));
}

TEST(SplitBucket, cannotMergeIncompatiblePattern) {
  const auto a = make("a##b", {3, 2});
  const auto b = make("b##b", {1, 5});
  ASSERT_FALSE(a.canMerge(b));
}

TEST(SplitBucket, cannotMergeIncompatibleIndices) {
  const auto a = make("a##b", {3, 2});
  const auto b = make("a###b", {2});
  ASSERT_FALSE(a.canMerge(b));
}

TEST(SplitBucket, canMerge) {
  auto a = make("a##b", {3, 2});
  auto b = make("a###b", {1, 5});
  ASSERT_TRUE(a.canMerge(b));
  const auto merged = SplitBucket(a, b);
  EXPECT_EQ(merged.pattern, "a#b");
  EXPECT_EQ(merged.sortedIndices, Indices({1, 2, 3, 5}));
}

TEST(SplitBucket, packEmpty) {
  auto a = make("a#b", {});
  a.pack();
  EXPECT_EQ(a.ranges, (Ranges{}));
  EXPECT_EQ(a.step, -1);
}

TEST(SplitBucket, pack) {
  auto a = make("a#b", {1, 2, 3, 4});
  a.pack();
  EXPECT_EQ(a.ranges, (Ranges{{1, 4}}));
  EXPECT_EQ(a.step, 1);
}

TEST(SplitBucket, packDisjoint) {
  auto a = make("a#b", {1, 2, 6, 7});
  a.pack();
  EXPECT_EQ(a.ranges, (Ranges{{1, 2}, {6, 7}}));
  EXPECT_EQ(a.step, 1);
}

TEST(SplitBucket, packStep2) {
  auto a = make("a#b", {1, 3, 5, 7});
  a.pack();
  EXPECT_EQ(a.ranges, (Ranges{{1, 7}}));
  EXPECT_EQ(a.step, 2);
}

TEST(SplitBucket, packStep2Disjoint) {
  auto a = make("a#b", {1, 3, 5, 7, 21, 23, 25});
  a.pack();
  EXPECT_EQ(a.ranges, (Ranges{{1, 7}, {21, 25}}));
  EXPECT_EQ(a.step, 2);
}

TEST(SplitBucket, packStep3) {
  auto a = make("a#b", {0, 3, 6});
  a.pack();
  EXPECT_EQ(a.ranges, (Ranges{{0, 6}}));
  EXPECT_EQ(a.step, 3);
}

TEST(SplitBucket, outputFile) {
  make("abc", {}).output(true /*bake*/, [](Item item) {
    EXPECT_EQ(item.filename, "abc");
    EXPECT_EQ(item.getType(), Item::SINGLE);
    EXPECT_EQ(item.start, -1);
    EXPECT_EQ(item.end, -1);
    EXPECT_EQ(item.step, -1);
  });
}

TEST(SplitBucket, outputSingleBaked) {
  make("a##b", {3})
      .output(true /*bake*/, [](Item item) {
        EXPECT_EQ(item.filename, "a03b");
        EXPECT_EQ(item.getType(), Item::SINGLE);
        EXPECT_EQ(item.start, -1);
        EXPECT_EQ(item.end, -1);
        EXPECT_EQ(item.step, -1);
      });
}

TEST(SplitBucket, outputSingleBakedNoPadding) {
  make("a#b", {123})
      .output(true /*bake*/, [](Item item) {
        EXPECT_EQ(item.filename, "a123b");
        EXPECT_EQ(item.getType(), Item::SINGLE);
        EXPECT_EQ(item.start, -1);
        EXPECT_EQ(item.end, -1);
        EXPECT_EQ(item.step, -1);
      });
}

TEST(SplitBucket, outputRangeBakeSingleton) {
  make("a#b", {1, 2})
      .output(true /*bake*/, [](Item item) {
        EXPECT_EQ(item.filename, "a#b");
        EXPECT_EQ(item.getType(), Item::INDICED);
        EXPECT_EQ(item.indices, Indices({1, 2}));
        EXPECT_EQ(item.start, -1);
        EXPECT_EQ(item.end, -1);
        EXPECT_EQ(item.step, -1);
      });
}

TEST(SplitBucket, outputRangeNoBakeSingleton) {
  make("a##b", {3, 4})
      .output(false /*bake*/, [](Item item) {
        EXPECT_EQ(item.filename, "a##b");
        EXPECT_EQ(item.getType(), Item::INDICED);
        EXPECT_EQ(item.indices, Indices({3, 4}));
        EXPECT_EQ(item.start, -1);
        EXPECT_EQ(item.end, -1);
        EXPECT_EQ(item.step, -1);
      });
}

TEST(SplitBucket, outputPackedRange) {
  makeAndPack("a##b", {3, 4})
      .output(false /*bake*/, [](Item item) {
        EXPECT_EQ(item.filename, "a##b");
        EXPECT_EQ(item.getType(), Item::PACKED);
        EXPECT_EQ(item.indices, Indices());
        EXPECT_EQ(item.start, 3);
        EXPECT_EQ(item.end, 4);
        EXPECT_EQ(item.step, 1);
      });
}

TEST(FileBucketizer, ingest) {
  FileBucketizer bucketizer;
  std::string output;

  output = "p1/numbers1_5.jpg";
  auto &result1 = bucketizer.ingest(output);
  EXPECT_EQ(result1.pattern, "p1/numbers#_#.jpg");
  EXPECT_EQ(result1.columns.size(), 2);
  EXPECT_EQ(result1.columns[0], Indices({1}));
  EXPECT_EQ(result1.columns[1], Indices({5}));

  output = "p1/numbers1_6.jpg";
  auto &result2 = bucketizer.ingest(output);
  EXPECT_EQ(result2.pattern, "p1/numbers#_#.jpg");
  EXPECT_EQ(result2.columns.size(), 2);
  EXPECT_EQ(result2.columns[0], Indices({1, 1}));
  EXPECT_EQ(result2.columns[1], Indices({5, 6}));
  EXPECT_EQ(&result1, &result2);
}

TEST(FileBucketizer, transfert) {
  FileBucketizer bucketizer;
  std::string output;

  output = "numbers1_5.jpg";
  bucketizer.ingest(output);

  output = "numbers1_6.jpg";
  bucketizer.ingest(output);

  const Buckets v(bucketizer.transfer());

  ASSERT_EQ(v.size(), 1);
  const auto &bucket = v[0];
  EXPECT_EQ(bucket.pattern, "numbers#_#.jpg");
  EXPECT_EQ(bucket.columns.size(), 2);
  EXPECT_EQ(bucket.columns[0], Indices({1, 1}));
  EXPECT_EQ(bucket.columns[1], Indices({5, 6}));
}
} // namespace details
} // namespace sequence
