#include "sequence/Item.hpp"

#include <gtest/gtest.h>

#include "sequence/details/ParserUtils.hpp"

namespace sequence {
namespace details {

Bucket getBucket() {
  Bucket bucket;
  bucket.pattern = "/path_101/file-##-##.jpg";
  bucket.columns.push_back(Indices({1, 1, 2, 3}));
  bucket.columns.push_back(Indices({1, 2, 2, 2}));
  return bucket;
}

Buckets asVector(Bucket bucket) {
  Buckets result;
  result.push_back(std::move(bucket));
  return result;
}

TEST(retainNone, simple) { EXPECT_EQ(retainNone(getBucket()), LOCATION_NONE); }

TEST(retainFirst, simple) { EXPECT_EQ(retainFirst(getBucket()), 1); }

TEST(retainLast, simple) { EXPECT_EQ(retainLast(getBucket()), 0); }

TEST(retainHighestVariance, simple) {
  EXPECT_EQ(retainHighestVariance(getBucket()), 1);
}

SplitBuckets splitAndSort(SplitIndexStrategy stategy,
                          Bucket bucket = getBucket()) {
  Buckets buckets;
  buckets.push_back(std::move(bucket));
  return splitAllAndSort(stategy, std::move(buckets));
}

TEST(splitAll, retainNone) {
  const auto results = splitAndSort(RETAIN_NONE);
  ASSERT_EQ(results.size(), 4);
  EXPECT_EQ(results[0].pattern, "/path_101/file-01-01.jpg");
  EXPECT_EQ(results[1].pattern, "/path_101/file-01-02.jpg");
  EXPECT_EQ(results[2].pattern, "/path_101/file-02-02.jpg");
  EXPECT_EQ(results[3].pattern, "/path_101/file-03-02.jpg");
}

TEST(splitAll, retainFirst) {
  const auto results = splitAndSort(RETAIN_FIRST_LOCATION);
  ASSERT_EQ(results.size(), 2);
  EXPECT_EQ(results[0].pattern, "/path_101/file-##-02.jpg");
  EXPECT_EQ(results[0].sortedIndices, Indices({1, 2, 3}));
  EXPECT_EQ(results[1].pattern, "/path_101/file-01-01.jpg");
  EXPECT_EQ(results[1].sortedIndices, Indices());
}

TEST(splitAll, retainLast) {
  const auto results = splitAndSort(RETAIN_LAST_LOCATION);
  ASSERT_EQ(results.size(), 3);
  EXPECT_EQ(results[0].pattern, "/path_101/file-01-##.jpg");
  EXPECT_EQ(results[0].sortedIndices, Indices({1, 2}));
  EXPECT_EQ(results[1].pattern, "/path_101/file-02-02.jpg");
  EXPECT_EQ(results[1].sortedIndices, Indices());
  EXPECT_EQ(results[2].pattern, "/path_101/file-03-02.jpg");
  EXPECT_EQ(results[2].sortedIndices, Indices());
}

TEST(splitAll, retainHighestVariance) {
  const auto results = splitAndSort(RETAIN_HIGHEST_VARIANCE);
  ASSERT_EQ(results.size(), 2);
  EXPECT_EQ(results[0].pattern, "/path_101/file-##-02.jpg");
  EXPECT_EQ(results[0].sortedIndices, Indices({1, 2, 3}));
  EXPECT_EQ(results[1].pattern, "/path_101/file-01-01.jpg");
  EXPECT_EQ(results[1].sortedIndices, Indices());
}

TEST(SplitBucket, outputSingleMultiplePlaceholder) {
  Bucket bucket("a#b####c");
  bucket.columns.push_back({1});
  bucket.columns.push_back({2010});
  const auto results = splitAndSort(RETAIN_HIGHEST_VARIANCE, std::move(bucket));
  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0].pattern, "a1b2010c");
  EXPECT_EQ(results[0].sortedIndices, Indices());
}

TEST(noPaddingLess, identity) {
  const Bucket a("ab##c");
  EXPECT_FALSE(noPaddingLess(a, a));
}

TEST(noPaddingLess, differentPadding) {
  const Bucket a("ab##c");
  const Bucket b("ab#c");
  EXPECT_FALSE(noPaddingLess(a, b));
  EXPECT_FALSE(noPaddingLess(b, a));
}

TEST(noPaddingLess, differentPattern) {
  const Bucket a("a##c");
  const Bucket b("ab##c");
  EXPECT_TRUE(noPaddingLess(a, b));
  EXPECT_FALSE(noPaddingLess(b, a));
}
} // namespace details
} // namespace sequence
