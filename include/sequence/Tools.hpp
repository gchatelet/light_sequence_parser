#ifndef TOOLS_HPP_
#define TOOLS_HPP_

#include <sequence/Item.hpp>

#include <regex>

namespace sequence {

/**
 * Create an Item representing a single file.
 * Returns an invalid Item if filename contains a padding character
 * ('#' or '@').
 */
Item createSingleFile(std::string filename);

/**
 * Create an Item representing a packed sequence.
 * Returns an invalid Item if prefix or suffix contains a padding character
 * ('#' or '@').
 *
 * eg: "filename-###.png"
 *      ----^---- ^ --^-
 *       prefix   | suffix
 *                |
 *             padding
 */
Item createSequence(std::string prefix, std::string suffix, index_type start, index_type end, unsigned char padding = 0, unsigned char step = 1);

/**
 * Produces a matcher from a pattern string.
 * Use it with the following match function.
 *
 * eg: "file@.png", "file###-tmp.png", "file###*"
 *
 * throws std::invalid_argument if string is empty or does not contain a
 * padding character ('#' or '@').
 */
std::regex getMatcher(const std::string& pattern, bool ignoreCase = false);

/**
 * Tests if a particular Item matches a pattern.
 * Useful to filter a collection of Items. eg:
 *
 * const std::regex matcher = getMatcher("file-@.*");
 *
 * Items items = ...
 * // browsing items
 * for(const auto& candidate : items)
 *   if(match(matcher, candidate))
 *     std::cout << candidate.filename << std::endl;
 *
 * // filtering out unmatched items
 * auto predicate = [&](const Item& item) -> bool {
 *   return !match(matcher, item);
 * };
 * items.erase(std::remove_if(items.begin(), items.end(), predicate), items.end());
 */
bool match(const std::regex& matcher, const Item& candidate);

}  // namespace sequence

#endif /* TOOLS_HPP_ */
