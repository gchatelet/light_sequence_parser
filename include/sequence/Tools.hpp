#ifndef TOOLS_HPP_
#define TOOLS_HPP_

#include <sequence/Item.hpp>
#include <sequence/details/StringView.hpp>

#include <regex>
#include <string>

namespace sequence {

// Create a pattern composed from prefix, suffix and padding count.
// If padding is 0, a single # is output.
// If padding is more than MAX_PADDING, a domain_error is thrown.
//
// eg: "filename-###.png"
//      ----^---- ^ --^-
//       prefix   | suffix
//                |
//             padding
std::string createPattern(CStringView prefix, CStringView suffix,
                          unsigned char padding = 1);

// Extracts prefix and suffix from pattern.
// If pattern is malformed (contains no padding character or contains more than
// one sequence of padding) an invalid_argument is thrown.
std::pair<CStringView, CStringView> getPrefixAndSuffix(CStringView pattern);

// Returns the number of padding characters between 1 and MAX_PADDING.
// If pattern is malformed (contains no padding character or contains more than
// one sequence of padding) an invalid_argument is thrown.
unsigned getPadding(CStringView pattern);

// Create an Item representing a single file.
// Returns an invalid Item if filename contains a padding character ('#' or
// '@').
Item createSingleFile(CStringView filename);

// Create an Item representing a packed sequence.
// Returns an invalid Item if prefix or suffix contains a padding character
// ('#' or '@').
//
// eg: "filename-###.png"
//      ----^---- ^ --^-
//       prefix   | suffix
//                |
//             padding
Item createSequence(CStringView pattern, Index start, Index end,
                    unsigned char step = 1);

Item createSequence(CStringView pattern, Indices indices);

// Produces a matcher from a pattern string.
// Use it with the following match function.
//
// eg: "file@.png", "file###-tmp.png", "file###*"
//
// throws std::invalid_argument if string is empty or does not contain a padding
// character ('#' or '@').
std::regex getMatcher(CStringView pattern, bool ignoreCase = false);

// Tests if a particular Item matches a pattern.
// Useful to filter a collection of Items. eg:
//
// const std::regex matcher = getMatcher("file-@.*");
//
// Items items = ...
// // browsing items
// for(const auto& candidate : items)
//   if(match(matcher, candidate))
//     std::cout << candidate.filename << std::endl;
//
// // filtering out unmatched items
// auto predicate = [&](const Item& item) -> bool {
//   return !match(matcher, item);
// };
// items.erase(std::remove_if(items.begin(), items.end(), predicate),
// items.end());
bool match(const std::regex &matcher, const Item &candidate);

namespace details {
std::string getMatcherString(std::string pattern);
} // namespace details

} // namespace sequence

#endif /* TOOLS_HPP_ */
