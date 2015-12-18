#include "sequence/details/ParserUtils.hpp"

namespace sequence {
namespace details {

size_t retainNone(const Bucket &bucket) {
  assert(!bucket.columns.empty());
  return LOCATION_NONE;
}

size_t retainFirst(const Bucket &bucket) {
  assert(!bucket.columns.empty());
  return bucket.columns.size() - 1;
}

size_t retainLast(const Bucket &bucket) {
  assert(!bucket.columns.empty());
  return 0;
}

size_t retainHighestVariance(const Bucket &bucket) {
  assert(!bucket.columns.empty());
  size_t lowestVarianceIndex = LOCATION_NONE;
  size_t lowestVariance = std::numeric_limits<size_t>::max();
  for (size_t i = 0; i < bucket.columns.size(); ++i) {
    const size_t current = estimateDistinctIndices(bucket.columns[i]);
    if (current < lowestVariance) {
      lowestVariance = current;
      lowestVarianceIndex = i;
    }
  }
  return lowestVarianceIndex;
}

size_t getPivotIndex(const SplitIndexStrategy strategy, const Bucket &bucket) {
  switch (strategy) {
  case RETAIN_NONE:
    return retainNone(bucket);
  case RETAIN_LAST_LOCATION:
    return retainLast(bucket);
  case RETAIN_FIRST_LOCATION:
    return retainFirst(bucket);
  case RETAIN_HIGHEST_VARIANCE:
    return retainHighestVariance(bucket);
  default:
    assert(0);
  }
}

SplitBuckets splitAllAndSort(const SplitIndexStrategy strategy,
                             Buckets splittable_buckets) {
  SplitBuckets buckets;
  const auto pusher = [&splittable_buckets](Bucket b) {
    splittable_buckets.push_back(std::move(b));
  };
  // Recursively splitting buckets.
  while (!splittable_buckets.empty()) {
    Bucket bucket = std::move(splittable_buckets.back());
    splittable_buckets.pop_back();
    if (bucket.splittable()) {
      const size_t index = getPivotIndex(strategy, bucket);
      if (index == LOCATION_NONE) {
        bucket.flatten(pusher);
      } else {
        bucket.split(index, pusher);
      }
    } else {
      if (bucket.single()) {
        bucket.flatten(pusher);
      } else {
        buckets.emplace_back(std::move(bucket));
      }
    }
  }
  std::sort(std::begin(buckets), std::end(buckets));
  return buckets;
}

// Adapted from std::unique.
// http://en.cppreference.com/w/cpp/algorithm/unique
template <class ForwardIt> ForwardIt merge(ForwardIt first, ForwardIt last) {
  if (first == last)
    return last;

  ForwardIt result = first;
  while (++first != last) {
    if (result->canMerge(*first)) {
      *result = SplitBucket(*result, *first);
    } else if (++result != first) {
      *result = std::move(*first);
    }
  }
  return ++result;
}

// This is implemented as a sort + unique algorithm with predicate comparing
// patterns without taking padding into account.
// When two buckets compares equals (i.e. to be merged) and the indices are
// compatible buckets are merged.
void mergeCompatiblePadding(SplitBuckets &buckets) {
  assert(std::is_sorted(std::begin(buckets), std::end(buckets)));
  buckets.erase(merge(std::begin(buckets), std::end(buckets)),
                std::end(buckets));
}

bool noPaddingLess(const Bucket &a, const Bucket &b) {
  return getInternalPrefixAndSuffix(a.pattern) <
         getInternalPrefixAndSuffix(b.pattern);
}

} // namespace details
} // namespace sequence
