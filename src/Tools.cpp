#include <sequence/Tools.hpp>

#include <cassert>

#include <algorithm>

#include <sequence/details/StringUtils.hpp>

namespace sequence {

namespace {

bool containsPatternCharacter(CStringView str) { return str.contains('#'); }

void replaceAll(std::string &subject, const std::string &search,
                const std::string &replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
}

} // namespace

std::string createPattern(CStringView prefix, CStringView suffix,
                          unsigned char padding) {
  if (padding > MAX_PADDING) {
    throw std::domain_error("padding should be <= MAX_PADDING");
  }
  if (padding == 0) {
    padding = 1;
  }
  std::array<char, MAX_PADDING> buffer;
  std::fill(buffer.begin(), buffer.end(), PADDING_CHAR);
  return concat(prefix, CStringView(buffer.cbegin(), padding), suffix);
}

std::pair<CStringView, CStringView> getPrefixAndSuffix(CStringView pattern) {
  const auto first = pattern.indexOf(PADDING_CHAR);
  if (first == CStringView::npos) {
    throw std::invalid_argument("no padding character found in pattern");
  }
  const auto last = pattern.lastIndexOf(PADDING_CHAR);
  assert(last != CStringView::npos);
  const CStringView padding = pattern.substr(first, last - first + 1);
  if (std::any_of(padding.begin(), padding.end(),
                  [](char c) { return c != PADDING_CHAR; })) {
    throw std::invalid_argument("multiple padding found in pattern");
  }
  if (padding.size() > MAX_PADDING) {
    throw std::invalid_argument("padding too large found in pattern");
  }
  return {pattern.substr(0, first), pattern.substr(last + 1)};
}

unsigned getPadding(CStringView pattern) {
  const auto pair = getPrefixAndSuffix(pattern);
  return pattern.size() - pair.first.size() - pair.second.size();
}

Item createSingleFile(CStringView filename) {
  if (containsPatternCharacter(filename))
    return {};
  return Item{filename.toString()};
}

Item createSequence(CStringView pattern, Index start, Index end,
                    unsigned char step) {
  if (step == 0)
    return {};
  if (end < start)
    return {};
  Item item(pattern);
  item.padding = getPadding(pattern);
  item.step = step;
  item.start = start;
  item.end = end;
  return item;
}

Item createSequence(CStringView pattern, Indices indices) {
  return Item(pattern, std::move(indices));
}

std::regex getMatcher(CStringView pattern, bool ignoreCase) {
  auto flags = std::regex_constants::ECMAScript;
  if (ignoreCase)
    flags |= std::regex_constants::icase;
  return std::regex(details::getMatcherString(pattern.toString()), flags);
}

bool match(const std::regex &matcher, const Item &candidate) {
  return std::regex_match(candidate.filename, matcher);
}

namespace details {
std::string getMatcherString(std::string pattern) {
  if (pattern.empty())
    throw std::invalid_argument("empty pattern");
  // replacing @ by #
  std::replace(pattern.begin(), pattern.end(), '@', '#');
  const auto padding = std::count(pattern.begin(), pattern.end(), '#');
  if (padding == 0)
    throw std::invalid_argument(
        "pattern should contain a padding character '#' or '@'");
  replaceAll(pattern, ".", "\\.");
  replaceAll(pattern, "*", ".*");
  if (padding == 1)
    replaceAll(pattern, "#", "#+");
  return pattern;
}

} // namespace details
} // namespace sequence
