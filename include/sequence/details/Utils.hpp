#pragma once

#include <cassert>
#include <cstdint>

#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <functional>

#include "sequence/Common.hpp"
#include "sequence/Item.hpp"
#include "sequence/details/StringView.hpp"

namespace sequence {
namespace details {

// Given a CStringView containing only digits, gives you the corresponding
// index. If the integer is too big to fit in Index overflowed if set to true
// and index contains garbage.
//
// e.g. IndexParser("123").index == 123
struct IndexParser {
  Index index = 0;
  bool overflowed = false;

  IndexParser(CStringView c);

private:
  void put(const char c);
};

// Extract substrings where predicate is true into ranges.
template <typename View>
std::vector<View> ranges(View view, std::function<bool(const char)> predicate);

// Extract substrings matching integers into ranges.
std::vector<StringView> integerRanges(StringView view);

// Given a path, extracts the integer from the filename and set replace them
// with '#'.
// Numbers too big to be converted to Index are left untouched.
void extractFileIndicesAndNormalize(StringView filename, Indices &indices);

// Write 0 padded value into placeholder.
void bake(Index value, StringView placeholder);

// Returns a vector of views within pattern corresponding to consecutive '#'.
// e.g. "path##.ext" -> {"##"}
std::vector<StringView> getPlaceholders(StringView pattern);

// Returns a vector of views within pattern corresponding to consecutive '#'.
// e.g. "path##.ext" -> {"path", ".ext"}
std::vector<CStringView> getText(CStringView pattern);

// Given a pattern with '#', replaces the 'index' nth placeholder with the
// specified value.
// e.g. bake("/path/to/file##_###.cr#", 1, 12) == "/path/to/file##_012.cr#"
void bake(StringView pattern, size_t index, Index value);

// Approximate calculation of the number of distinct elements in indices.
// This estimator is precise for less than 1K elements, then it starts getting
// less precise: 10K typically returns around 9.7K.
// It cannot count above 2^18 ~ 262K.
// It is fast and consumes 32KiB memory overall.
// Complexity is O(N).
size_t estimateDistinctIndices(const Indices &indices);

// Groups all indices for a particular pattern.
// pattern: a string with '#' in place of digits in the filename
// e.g. "/path/to/sequence/file##_###.cr#"
// This pattern would have three columns, each column gathers integers for its
// placeholder.
struct Bucket {
  std::string pattern;
  std::vector<Indices> columns;

  Bucket() = default;
  Bucket(Bucket &&) = default;
  Bucket &operator=(Bucket &&) = default;
  Bucket(CStringView str) : pattern(str.toString()) {}

  Bucket(const Bucket &) = delete;
  Bucket &operator=(const Bucket &) = delete;

  // Adds indices to this pattern.
  // Either columns.empty() or columns.size() == indices.size().
  void ingest(const Indices &indices);

  // Returns whether this pattern represents a single value.
  // i.e. many columns with a single value.
  bool single() const;

  // Returns whether this pattern is splittable.
  // i.e. many columns with more than one value.
  bool splittable() const;

  // Splits this Bucket according to column 'index'.
  // Pushes as many Bucket as distinct values in the split column.
  void split(size_t index, std::function<void(Bucket)> push) const;

  // Pushes as many Buckets as there are values, baking the indices in the file.
  void flatten(std::function<void(Bucket)> push) const;
};

typedef std::vector<Bucket> Buckets;

struct Range {
  Index start;
  Index end;

  Range() = default;
  Range(Index start, Index end) : start(start), end(end) {}

  bool operator==(const Range &o) const {
    return start == o.start && end == o.end;
  }
};

typedef std::vector<Range> Ranges;

// A bucket which contains a single column.
// It is created from a fully splitted bucket.
// Indices are guaranteed to be split.
struct SplitBucket {
  std::string pattern;
  Indices sortedIndices;
  char step = -1;
  Ranges ranges;

  SplitBucket() = default;
  SplitBucket(SplitBucket &&) = default;
  SplitBucket &operator=(SplitBucket &&) = default;

  SplitBucket(Bucket &&bucket);
  SplitBucket(SplitBucket &a, SplitBucket &b);

  bool containsPadding() const;
  bool canMerge(const SplitBucket &other) const;
  std::string getBakedPattern(Index value) const;
  void pack();
  void output(bool bakeSingleton, std::function<void(Item)> push);

  bool operator<(const SplitBucket &other) const {
    return pattern < other.pattern;
  }
};

// Returns the step of the sorted sequence or -1.
char getStep(Indices sortedIndices);

SplitBucket mergeSplitBucket(SplitBucket &a, SplitBucket &b);

typedef std::vector<SplitBucket> SplitBuckets;

template <typename T> void sortIfNeeded(std::vector<T> &elements);
template <typename T, class Compare>
void sortIfNeeded(std::vector<T> &elements, Compare comp);

struct FileBucketizer {
  // Ingest this path into a bucket, extracting the pattern and integers from
  // the filename.
  Bucket &ingest(StringView string);

  // Retrieve all the buckets and resets the FileBucketizer.
  std::vector<Bucket> transfer();

private:
  Bucket &getOrAdd(CStringView string, uint32_t seed);

  std::unordered_map<uint32_t, std::vector<Bucket>> hash_map;
  Indices tmp;
};

////////////////////////////////////////////////////////////////////////////////

template <typename View>
std::vector<View> ranges(View view, std::function<bool(const char)> predicate) {
  std::vector<View> output;
  size_t in = StringView::npos;
  for (size_t i = 0; i < view.size(); ++i) {
    const bool inRange = predicate(view[i]);
    if (inRange && in == StringView::npos) {
      in = i;
    }
    if (!inRange && in != StringView::npos) {
      output.push_back(view.substr(in, i - in));
      in = StringView::npos;
    }
  }
  if (in != StringView::npos) {
    output.push_back(view.substr(in));
  }
  return output;
}

template <typename T> void sortIfNeeded(std::vector<T> &elements) {
  if (!std::is_sorted(std::begin(elements), std::end(elements))) {
    std::sort(std::begin(elements), std::end(elements));
  }
}

template <typename T, class Compare>
void sortIfNeeded(std::vector<T> &elements, Compare comp) {
  if (!std::is_sorted(std::begin(elements), std::end(elements)), comp) {
    std::sort(std::begin(elements), std::end(elements), comp);
  }
}
} // namespace details
} // namespace sequence
