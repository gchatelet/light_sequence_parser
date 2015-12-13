#pragma once

#include "sequence/Parser.hpp"
#include "sequence/details/Utils.hpp"

namespace sequence {
namespace details {

enum : size_t { LOCATION_NONE = std::numeric_limits<size_t>::max() };

size_t retainNone(const Bucket &bucket);
size_t retainFirst(const Bucket &bucket);
size_t retainLast(const Bucket &bucket);
size_t retainHighestVariance(const Bucket &bucket);

size_t getPivotIndex(const SplitIndexStrategy strategy, const Bucket &bucket);

// Recursively split buckets and sort the outcome.
SplitBuckets splitAllAndSort(const SplitIndexStrategy strategy,
                             Buckets splittable_buckets);

// Merges buckets with same filename but different paddings.
// Buckets must have only one column.
void mergeCompatiblePadding(SplitBuckets &buckets);

void bakeSingleton(SplitBuckets &bucket);

// Compares two buckets not tacking padding into account.
// This function is a bit costly and can be improved if needed.
bool noPaddingLess(const Bucket &a, const Bucket &b);

} // namespace details
} // namespace sequence
