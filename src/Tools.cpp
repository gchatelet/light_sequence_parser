#include <sequence/Tools.hpp>

#include <algorithm>

namespace sequence {

namespace {

bool containsPatternCharacter(const std::string &str) {
  return str.find_first_of("#@") != std::string::npos;
}

void replaceAll(std::string &subject, const std::string &search,
                const std::string &replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
}

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

} // namespace

Item createSingleFile(std::string filename) {
  if (containsPatternCharacter(filename))
    return {};
  return {std::move(filename)};
}

Item createSequence(std::string prefix, std::string suffix, index_type start,
                    index_type end, unsigned char padding, unsigned char step) {
  if (step == 0)
    return {};
  if (containsPatternCharacter(prefix) || containsPatternCharacter(suffix))
    return {};
  if (end < start)
    return {};
  if (padding == 0)
    padding = 1;
  prefix.reserve(prefix.size() + suffix.size() + padding);
  for (unsigned i = 0; i < padding; ++i)
    prefix += '#';
  prefix += suffix;
  Item item(prefix);
  item.padding = padding;
  item.step = step;
  item.start = start;
  item.end = end;
  return item;
}

std::regex getMatcher(const std::string &pattern, bool ignoreCase) {
  auto flags = std::regex_constants::basic;
  if (ignoreCase)
    flags |= std::regex_constants::icase;
  return std::regex(getMatcherString(pattern), flags);
}

bool match(const std::regex &matcher, const Item &candidate) {
  return std::regex_match(candidate.filename, matcher);
}

} // namespace sequence
