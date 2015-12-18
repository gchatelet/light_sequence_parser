#include "sequence/details/Utils.hpp"

#include <cassert>
#include <cmath>
#include <cstring>

#include <algorithm>
#include <bitset>
#include <set>
#include <utility>

#include "sequence/Tools.hpp"
#include "sequence/details/Hash.hpp"
#include "sequence/details/StringView.hpp"
#include "sequence/details/StringUtils.hpp"

namespace sequence {
namespace details {
#if defined(_WIN64) || defined(_WIN32)
#define PATH_SEPARATOR '\\'
#elif defined(__APPLE__) || defined(__linux)
#define PATH_SEPARATOR '/'
#else
#error "Unexpected platform"
#endif

IndexParser::IndexParser(CStringView str) {
  for (const char c : str) {
    put(c);
  }
}

void IndexParser::put(const char c) {
  assert(isdigit(c));
  const Index diff = c - '0';
  overflowed |= index > (std::numeric_limits<uint32_t>::max() / 10);
  index *= 10;
  overflowed |= index > (std::numeric_limits<uint32_t>::max() - diff);
  index += diff;
}

std::vector<StringView> integerRanges(StringView view) {
  return ranges(view, [](const char c) { return isdigit(c); });
}

void extractFileIndicesAndNormalize(StringView path, Indices &indices) {
  const auto npos = CStringView::npos;
  indices.clear();
  const size_t lastSeparator = path.lastIndexOf(PATH_SEPARATOR);
  const size_t fileIndex = lastSeparator == npos ? 0 : lastSeparator + 1;
  const size_t lastDot = path.lastIndexOf('.');
  const char *lastDotPtr =
      lastDot == npos ? path.end() : path.begin() + lastDot;
  const std::vector<StringView> integers(integerRanges(path.substr(fileIndex)));
  for (StringView integer : integers) {
    if (integer.begin() > lastDotPtr) {
      continue;
    }
    IndexParser parser(integer);
    if (!parser.overflowed) {
      indices.push_back(parser.index);
      memset(integer.ptr(), PADDING_CHAR, integer.size());
    }
  }
}

void bake(Index value, StringView placeholder) {
  const auto pStart = placeholder.begin();
  for (auto pEnd = placeholder.end(); pEnd != pStart;) {
    --pEnd;
    *pEnd = '0' + value % 10;
    value /= 10;
  }
}

std::vector<StringView> getPlaceholders(StringView pattern) {
  return ranges(pattern, [](char c) { return c == PADDING_CHAR; });
}

std::vector<CStringView> getText(CStringView pattern) {
  return ranges(pattern, [](char c) { return c != PADDING_CHAR; });
}

void bake(StringView pattern, size_t index, Index value) {
  const auto placeholders = getPlaceholders(pattern);
  assert(index < placeholders.size());
  bake(value, placeholders[index]);
}

size_t estimateDistinctIndices(const Indices &indices) {
  constexpr size_t bits = 18; // 262 144 combinations, 32kiB
  constexpr size_t size = 1 << bits;
  constexpr uint32_t mask = size - 1;
  std::bitset<size> set; // this will consume size / 8 bytes
  for (const uint32_t value : indices) {
    set.set(hash(value) & mask);
  }
  return set.count();
}

void Bucket::ingest(const Indices &indices) {
  if (columns.empty()) {
    columns.resize(indices.size());
  }
  assert(columns.size() == indices.size());
  for (size_t i = 0; i < indices.size(); ++i) {
    columns[i].push_back(indices[i]);
  }
}

bool Bucket::single() const {
  return !columns.empty() && columns[0].size() == 1;
}

bool Bucket::splittable() const {
  return columns.size() > 1 && columns[0].size() > 1;
}

void Bucket::split(size_t index, std::function<void(Bucket)> push) const {
  assert(index < columns.size());
  const Indices &pivot = columns[index];
  std::unordered_map<Index, Bucket> map;
  Indices tmp;
  assert(columns.size() > 0);
  tmp.reserve(columns.size() - 1);
  for (size_t row = 0; row < pivot.size(); ++row) {
    const Index pivotValue = pivot[row];
    Bucket &reduced = map[pivotValue];
    if (reduced.pattern.empty()) {
      reduced.pattern = pattern;
      bake(reduced.pattern, index, pivotValue);
    }
    tmp.clear();
    for (size_t col = 0; col < columns.size(); ++col) {
      if (col != index) {
        tmp.push_back(columns[col][row]);
      }
    }
    reduced.ingest(tmp);
  }
  for (auto &pair : map) {
    push(std::move(pair.second));
  }
}

void Bucket::flatten(std::function<void(Bucket)> push) const {
  assert(columns.size() > 0);
  for (size_t row = 0; row < columns[0].size(); ++row) {
    Bucket file(pattern);
    auto placeholders = getPlaceholders(file.pattern);
    for (size_t col = 0; col < columns.size(); ++col) {
      bake(columns[col][row], placeholders[col]);
    }
    push(std::move(file));
  }
}

////////////////////////////////////////////////////////////////////////////////
SplitBucket::SplitBucket(Bucket &&bucket) {
  assert(!bucket.splittable());
  pattern = std::move(bucket.pattern);
  assert(bucket.columns.size() <= 1);
  if (bucket.columns.size() == 1) {
    sortedIndices = std::move(bucket.columns[0]);
    std::sort(std::begin(sortedIndices), std::end(sortedIndices));
  }
}

SplitBucket::SplitBucket(SplitBucket &a, SplitBucket &b) {
  assert(a.canMerge(b));
  CStringView prefix, suffix;
  std::tie(prefix, suffix) = getPrefixAndSuffix(a.pattern);
  pattern = concat(prefix, "#", suffix);
  sortedIndices = std::move(a.sortedIndices);
  sortedIndices.insert(std::end(sortedIndices), std::begin(b.sortedIndices),
                       std::end(b.sortedIndices));
  std::sort(std::begin(sortedIndices), std::end(sortedIndices));
}

struct FakeSet {
  typedef Index value_type;
  bool isDisjoint = true;
  void push_back(const Index &) { isDisjoint = false; }
};
bool SplitBucket::containsPadding() const {
  return CStringView(pattern).contains(PADDING_CHAR);
}
bool SplitBucket::canMerge(const SplitBucket &other) const {
  if (!containsPadding() || !other.containsPadding() ||
      getPrefixAndSuffix(pattern) != getPrefixAndSuffix(other.pattern)) {
    return false;
  }
  FakeSet set;
  std::set_intersection(std::begin(sortedIndices), std::end(sortedIndices),
                        std::begin(other.sortedIndices),
                        std::end(other.sortedIndices), std::back_inserter(set));
  return set.isDisjoint;
}

std::string SplitBucket::getBakedPattern(Index value) const {
  CStringView prefix, suffix;
  std::tie(prefix, suffix) = getPrefixAndSuffix(pattern);
  const auto padding = pattern.size() - prefix.size() - suffix.size();
  char buffer[10]; // 4,294,967,295 is 10 characters long maximum.
  StringView view(buffer, padding == 1 ? 1 + std::log10(value) : padding);
  bake(value, view);
  return concat(prefix, view, suffix);
}

char getStep(Indices sortedIndices) {
  bool minimum_step_set = false;
  size_t minimum_step = std::numeric_limits<char>::max();
  if (sortedIndices.size() >= 2) {
    for (auto previous = std::begin(sortedIndices), next = std::next(previous);
         next != std::end(sortedIndices); ++previous, ++next) {
      assert(*next > *previous);
      const size_t diff = *next - *previous;
      if (diff < minimum_step) {
        minimum_step = diff;
        minimum_step_set = true;
      }
    }
  }
  assert(minimum_step <= std::numeric_limits<char>::max());
  return minimum_step_set ? static_cast<char>(minimum_step) : -1;
}

template <typename Itr> Itr findRangeEnd(Itr first, Itr last, size_t step) {
  return std::adjacent_find(first, last, [step](const Index a, const Index b) {
    assert(b > a);
    return (b - a) != step;
  });
}

void SplitBucket::pack() {
  step = getStep(sortedIndices);
  if (step > 0) {
    auto rangeStart = std::begin(sortedIndices);
    const auto end = std::end(sortedIndices);
    for (auto rangeEnd = findRangeEnd(rangeStart, end, step); rangeEnd != end;
         rangeEnd = findRangeEnd(rangeStart, end, step)) {
      ranges.emplace_back(*rangeStart, *rangeEnd);
      rangeStart = std::next(rangeEnd);
    }
    ranges.emplace_back(*rangeStart, sortedIndices.back());
    sortedIndices.clear();
  }
}

void SplitBucket::output(bool bakeSingleton, std::function<void(Item)> push) {
  if (ranges.size()) { // PACKED items
    for (const auto range : ranges) {
      if (range.start == range.end && bakeSingleton) {
        push(createSingleFile(getBakedPattern(range.start)));
      } else {
        push(createSequence(pattern, range.start, range.end, step));
      }
    }
  } else { // INDICED items
    if (sortedIndices.size() > 1) {
      push(Item(pattern, std::move(sortedIndices)));
    } else if (sortedIndices.size() == 1) { // SINGLE items
      push(createSingleFile(getBakedPattern(sortedIndices[0])));
    } else if (sortedIndices.empty()) { // SINGLE items
      push(createSingleFile(pattern));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
Bucket &FileBucketizer::getOrAdd(CStringView bucket, uint32_t seed) {
  const uint32_t hashed = hash(bucket, seed);
  auto itr = hash_map.find(hashed);
  auto &vector = itr == hash_map.end() ? hash_map[hashed] : itr->second;
  for (auto &item : vector) {
    if (bucket == item.pattern) {
      return item;
    }
  }
  vector.emplace_back(bucket.toString());
  return vector.back();
}

Bucket &FileBucketizer::ingest(StringView filename) {
  extractFileIndicesAndNormalize(filename, tmp);
  auto &bucket = getOrAdd(filename, tmp.size());
  bucket.ingest(tmp);
  return bucket;
}

std::vector<Bucket> FileBucketizer::transfer() {
  std::vector<Bucket> dst;
  for (auto &pair : hash_map) {
    auto &src = pair.second;
    std::move(std::begin(src), std::end(src), std::back_inserter(dst));
  }
  hash_map.clear();
  return dst;
}

} // namespace details
} // namespace sequence
