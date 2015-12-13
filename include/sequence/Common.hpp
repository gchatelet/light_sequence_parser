#pragma once

#include <cstdint>
#include <vector>

namespace sequence {

typedef uint32_t Index;
typedef std::vector<Index> Indices;
enum : char { PADDING_CHAR = '#' };
enum : char { MAX_PADDING = 10 }; // 4,294,967,295

} // namespace sequence
