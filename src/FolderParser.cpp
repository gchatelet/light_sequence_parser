#include <sequence/Parser.hpp>

#ifdef GTEST
#include <gtest/gtest_prod.h>
#else
#define FRIEND_TEST(test_case_name, test_name)
#endif

#include <stdexcept>
#include <algorithm> // count, sort
#include <memory>    // unique_ptr
#include <map>
#include <set>
#include <queue>
#include <cassert>
#include <numeric>

namespace sequence {

namespace details {

namespace {

typedef typename STRING::iterator STRING_ITR;
typedef typename STRING::const_iterator STRING_CITR;

inline bool isDigit(CHAR c) { return c >= CHAR('0') && c <= CHAR('9'); }

template <typename ITR> inline ITR advanceToSharp(ITR itr) {
  while (*itr && *itr != Item::PADDING_CHAR)
    ++itr;
  return itr;
}

template <typename ITR> inline ITR advanceToNotSharp(ITR itr) {
  while (*itr && *itr == Item::PADDING_CHAR)
    ++itr;
  return itr;
}

inline void bake(const STRING_ITR pStart, STRING_ITR pEnd, index_type value) {
  while (pEnd != pStart) {
    --pEnd;
    *pEnd = '0' + value % 10;
    value /= 10;
  }
}

inline size_t countPadding(const STRING &pattern) {
  return count(pattern.begin(), pattern.end(), Item::PADDING_CHAR);
}

typedef std::pair<size_t, size_t> Location;
typedef std::vector<Location> Locations;

Locations getLocations(size_t count, const STRING &pattern) {
  Locations locations;
  locations.reserve(count);
  auto begin = pattern.begin();
  auto pCurrentPatternChar = begin;
  for (; count > 0; --count) {
    auto pPatternStart = advanceToSharp(pCurrentPatternChar);
    auto pPatternEnd = advanceToNotSharp(pPatternStart);
    locations.emplace_back(std::make_pair(std::distance(begin, pPatternStart),
                                          std::distance(begin, pPatternEnd)));
    pCurrentPatternChar = pPatternEnd;
  }
  return locations;
}

void bake(const Location &location, STRING &string, index_type value) {
  auto begin = string.begin();
  auto end = string.begin();
  std::advance(begin, location.first);
  std::advance(end, location.second);
  bake(begin, end, value);
}

} // namespace

struct PatternSet {
private:
  friend struct Trie;
  std::vector<VALUES> valueArrays;
  STRING pattern;

public:
  PatternSet(std::vector<VALUES> &&values) : valueArrays(std::move(values)) {}

  PatternSet(unsigned char slotCount, STRING &&pattern)
      : valueArrays(slotCount), pattern(std::move(pattern)) {}

  // Take values read in the path and add them to the data set
  void addLocationValues(const VALUES &values) {
    assert(values.size() == valueArrays.size());
    auto valuesItr = valueArrays.begin();
    for (auto start = values.begin(), end = values.end(); start != end;
         ++start, ++valuesItr) {
      valuesItr->push_back(*start);
    }
  }

  inline const std::vector<VALUES> &getLocationValues() const {
    return valueArrays;
  }

  inline const STRING &getPattern() const { return pattern; }

  inline const std::vector<VALUES> &getValuesArrays() const {
    return valueArrays;
  }

  void bakeConstantLocations() {
    auto pCurrentPatternChar = pattern.begin();
    for (auto &values : valueArrays) {
      auto copy = values;
      // extracting location from string
      auto pPatternStart = advanceToSharp(pCurrentPatternChar);
      auto pPatternEnd = advanceToNotSharp(pPatternStart);
      pCurrentPatternChar = pPatternEnd;
      // counting
      std::sort(copy.begin(), copy.end());
      auto uniqueItr = std::unique(copy.begin(), copy.end());
      auto count = std::distance(copy.begin(), uniqueItr);
      if (count == 1) {
        bake(pPatternStart, pPatternEnd, *copy.begin());
        values.clear();
      }
    }
    auto pBeginErase =
        std::remove_if(valueArrays.begin(), valueArrays.end(),
                       [](const VALUES &values) { return values.empty(); });
    valueArrays.erase(pBeginErase, valueArrays.end());
  }

  inline size_t getLocationCount() const { return valueArrays.size(); }

  // A data set is ready when there is one or less location left.
  // Otherwise it needs more splitting.
  inline bool isReady() const { return getLocationCount() <= 1; }

  inline bool isUnitFile() const { return valueArrays.empty(); }

private:
  FRIEND_TEST(PatternSet, notReadyAfterBaking);

  std::vector<std::unique_ptr<PatternSet>> split(size_t locationIndex) {
    assert(valueArrays.size() > 1);
    assert(locationIndex < valueArrays.size());
    const VALUES &pivot = valueArrays[locationIndex];
    const size_t locationCount = valueArrays.size();
    VALUES tmp;
    std::map<size_t, std::unique_ptr<PatternSet>> splitted;
    for (size_t i = 0, end = pivot.size(); i < end; ++i) {
      tmp.clear();
      for (const auto &values : valueArrays)
        tmp.push_back(values[i]);
      const auto value = pivot[i];
      auto pFound = splitted.find(value);
      if (pFound == splitted.end()) {
        pFound =
            splitted.insert( //
                        std::make_pair(
                            value, std::unique_ptr<PatternSet>(new PatternSet(
                                       locationCount, STRING(pattern)))))
                .first;
      }
      pFound->second->addLocationValues(tmp);
    }
    std::vector<std::unique_ptr<PatternSet>> result;
    for (auto &entry : splitted) {
      result.push_back(std::move(entry.second));
      result.back()->bakeConstantLocations();
    }
    return result;
  }

  FRIEND_TEST(Items, fromUnitFilePatternSet);
  FRIEND_TEST(Items, fromRegularPatternSet);
  FRIEND_TEST(Items, fromDegeneratedPatternSet);

  void addItems(Items &items) {
    if (isReady()) {
      if (isUnitFile()) { /* single file */
        items.emplace_back(std::move(pattern));
      } else { /* regular pattern */
        assert(valueArrays.size() == 1);
        assert(pattern.find(Item::PADDING_CHAR) != STRING::npos);
        const auto padding = countPadding(pattern);
        items.emplace_back(std::move(pattern), std::move(valueArrays[0]));
        items.back().padding = padding;
      }
    } else { /* baking every single values */
      const auto locations = getLocations(getLocationCount(), pattern);
      const size_t setSize = valueArrays[0].size();
      for (size_t i = 0; i < setSize; ++i) {
        for (size_t loc = 0; loc < locations.size(); ++loc) {
          bake(locations[loc], pattern, valueArrays[loc][i]);
        }
        items.emplace_back(pattern);
      }
    }
  }
};

struct IntegerParser {
  index_type current;
  bool overflowed;

  IntegerParser() : current(0), overflowed(false) {}

  inline void put(CHAR c) {
    assert(c >= '0' && c <= '9');
    const auto before = current;
    current *= 10;
    current += c - '0';
    overflowed |= (current / 10 != before);
  }

  inline void reset() {
    current = 0;
    overflowed = false;
  }
};

struct NumberExtractor {
  bool inDigit;
  IntegerParser parser;
  VALUES values;
  bool overflowed;

  NumberExtractor() : inDigit(false), overflowed(false) {}

  inline bool put(CHAR c) {
    const bool digit = isDigit(c);
    if (digit) {
      inDigit = true;
      parser.put(c);
    } else if (inDigit) {
      inDigit = false;
      values.push_back(parser.current);
      overflowed |= parser.overflowed;
      parser.reset();
    }
    return digit;
  }

  inline void clear() {
    values.clear();
    overflowed = false;
  }
};

struct Trie {
private:
  struct Node {
  private:
    typedef std::map<const CHAR, std::unique_ptr<Node>> MAP;
    typedef typename MAP::iterator Itr;
    MAP children;

  public:
    std::unique_ptr<PatternSet> pDataSet;
    Node *const pParent;
    const CHAR character;

    Node(Node *const pParent = nullptr, CHAR c = 0)
        : pParent(pParent), character(c) {}

    Node *get(const CHAR c) {
      auto pFound = children.find(c);
      if (pFound == children.end()) {
        pFound = children.insert(std::make_pair(c, std::unique_ptr<Node>(
                                                       new Node(this, c))))
                     .first;
      }
      return pFound->second.get();
    }

    bool put(const VALUES &values) {
      bool newNode = false;
      if (!pDataSet) {
        pDataSet.reset(new PatternSet(values.size(), buildPath(this)));
        newNode = true;
      }
      pDataSet->addLocationValues(values);
      return newNode;
    }

  private:
    static STRING buildPath(const Node *pNode) {
      STRING str;
      while (pNode->pParent) {
        str.push_back(pNode->character);
        pNode = pNode->pParent;
      }
      std::reverse(str.begin(), str.end());
      return str;
    }
  };

  static Node *ingestPath(NumberExtractor &extractor, Node *root,
                          const CHAR *pChar) {
    for (; *pChar != 0; ++pChar) {
      auto c = *pChar;
      if (extractor.put(c))
        c = Item::PADDING_CHAR;
      root = root->get(c);
    }
    extractor.put('\0'); // finishing the current number
    return root->put(extractor.values) ? root : nullptr;
  }

  Node root;
  NumberExtractor extractor;
  std::set<Node *> leaves;

public:
  bool push(CHAR const *pEntry) {
    auto pNewNode = ingestPath(extractor, &root, pEntry);
    const auto overflowed = extractor.overflowed;
    if (!overflowed && pNewNode) {
      leaves.insert(pNewNode);
    }
    extractor.clear();
    return overflowed;
  }

  typedef std::unique_ptr<PatternSet> UniquePatternSet;
  typedef std::vector<UniquePatternSet> PatternSets;
  typedef std::queue<UniquePatternSet> PatternSetQueue;

  PatternSetQueue dataSets() {
    PatternSetQueue dataSets;
    for (Node *pNode : leaves) {
      assert(pNode);
      assert(pNode->pDataSet.get());
      dataSets.push(std::move(pNode->pDataSet));
    }
    return dataSets;
  }

  Items reduceToIndicedItems(
      const std::function<int(const PatternSet &)> &splitIndexSelector) {
    Items items;
    PatternSetQueue toProcess = dataSets();
    for (; !toProcess.empty(); toProcess.pop()) {
      auto &patternSet = *(toProcess.front());
      if (patternSet.isReady()) {
        patternSet.addItems(items);
        continue;
      }
      const int splitIndex = splitIndexSelector(patternSet);
      if (splitIndex == -1) {
        patternSet.addItems(items);
        continue;
      }
      for (auto &&pSet : patternSet.split(splitIndex)) {
        toProcess.push(std::move(pSet));
      }
    }
    return items;
  }
};

namespace {

template <typename Container> bool sortIfNeeded(Container &container) {
  if (std::is_sorted(container.begin(), container.end()))
    return false;
  std::sort(container.begin(), container.end());
  return true;
}

template <typename Container, typename Predicate>
bool sortIfNeeded(Container &container, Predicate compare) {
  if (std::is_sorted(container.begin(), container.end(), compare))
    return false;
  std::sort(container.begin(), container.end(), compare);
  return true;
}

void eraseDuplicatedPaddingChar(std::string &a) {
  a.erase(std::unique(a.begin(), a.end(), [](CHAR a, CHAR b) {
            return a == b && a == Item::PADDING_CHAR;
          }), a.end());
}

// We want to merge items that are indiced are which share same prefix and
// suffix but which may have a different padding.
// e.g.
//  prefix##suffix  [1,99]
//  prefix###suffix [105]
bool merge(Item &itemA, Item &itemB) {
  if (itemA.getType() != Item::INDICED || itemB.getType() != Item::INDICED) {
    return false;
  }
  auto &a = itemA.filename;
  auto &b = itemB.filename;

  // Checking same prefix
  // prefix##suffix
  // prefix###suffix
  // ^------^
  const auto common = std::mismatch(a.begin(), a.end(), b.begin());

  // if one of the iterators is end, we can't merge
  auto itrA = common.first;
  auto itrB = common.second;
  if (itrA == a.end() || itrB == b.end())
    return false;

  // One the the iterators should point to PADDING_CHAR, otherwise we simply
  // don't have a common prefix
  // abc##suffix
  // abef###suffix
  // ^^
  if (*itrA != Item::PADDING_CHAR && *itrB != Item::PADDING_CHAR)
    return false;

  // Advancing pointers to move out of PADDING_CHAR
  itrA = advanceToNotSharp(itrA);
  itrB = advanceToNotSharp(itrB);

  // Now the rest should compare equal, otherwise we can't merge
  // prefix##suffix
  //         ^----^
  // prefix###suffix
  //          ^----^
  if (std::mismatch(itrA, a.end(), itrB) != std::make_pair(a.end(), b.end()))
    return false;

  // Checking if some indices appear in both containers
  VALUES allValues;
  // Appending all values
  allValues.reserve(itemA.indices.size() + itemB.indices.size());
  allValues.insert(allValues.end(), itemA.indices.begin(), itemA.indices.end());
  allValues.insert(allValues.end(), itemB.indices.begin(), itemB.indices.end());
  // Sorting if needed
  sortIfNeeded(allValues);
  // We must not have twice the same value in there
  if (std::adjacent_find(allValues.begin(), allValues.end()) != allValues.end())
    return false;

  // Now the two patterns are compatible appending b indices to a
  itemA.indices = std::move(allValues);

  // If a's padding is already 0 we're done.
  if (itemA.padding == 0)
    return true;

  // Otherwise setting a's padding to 0
  itemA.padding = 0;
  // and keeping only one Item::PADDING_CHAR
  eraseDuplicatedPaddingChar(a);
  return true;
}

void bakeSingleton(Item &item) {
  size_t indexToBake;
  switch (item.getType()) {
  case Item::INDICED: {
    auto &indices = item.indices;
    if (indices.size() != 1)
      return;
    indexToBake = indices[0];
    indices.clear();
    break;
  }
  case Item::PACKED: {
    if (item.start != item.end)
      return;
    indexToBake = item.start;
    item.step = -1;
    break;
  }
  default:
    return;
  }
  const auto locations = getLocations(1, item.filename);
  assert(locations.size() == 1);
  bake(locations[0], item.filename, indexToBake);
}

inline bool less(const Item &a, const Item &b) {
  std::string fileA(a.filename), fileB(b.filename);
  eraseDuplicatedPaddingChar(fileA);
  eraseDuplicatedPaddingChar(fileB);
  return fileA == fileB ? a.padding < b.padding : fileA < fileB;
}

void reduceToPackedItems(Item &item, std::function<void(Item &&)> push) {
  if (item.getType() == Item::SINGLE) {
    push(std::move(item));
    return;
  }
  if (item.padding == -1) {
    item.padding = countPadding(item.filename);
  }
  const auto &indices = item.indices;
  assert(!indices.empty());
  if (indices.size() == 1) {
    Item tmp;
    tmp.filename = item.filename;
    tmp.padding = item.padding;
    tmp.step = 1;
    tmp.start = indices[0];
    tmp.end = indices[0];
    push(std::move(tmp));
    return;
  }
  sortIfNeeded(item.indices);
  VALUES derivative(indices.size());
  std::adjacent_difference(indices.begin(), indices.end(), derivative.begin());
  assert(indices.size() >= 2);
  const auto step =
      std::max(index_type(1),
               *std::min_element(derivative.begin() + 1, derivative.end()));
  auto itr = indices.begin();
  typedef std::pair<size_t, size_t> Range;
  std::vector<Range> ranges;
  for (auto delta : derivative) {
    const auto current = *itr++;
    if (delta == step && !ranges.empty()) {
      ranges.back().second = current;
    } else {
      ranges.emplace_back(current, current);
    }
  }
  for (const auto &range : ranges) {
    Item tmp;
    tmp.filename = item.filename;
    tmp.padding = item.padding;
    tmp.step = step;
    tmp.start = range.first;
    tmp.end = range.second;
    push(std::move(tmp));
  }
}

int retainNone(const PatternSet &set) {
  assert(!set.isReady());
  return -1;
}

int retainLastLocation(const PatternSet &set) {
  assert(!set.isReady());
  return 0;
}

int retainHighestVariance(const PatternSet &set) {
  assert(!set.isReady());
  std::vector<size_t> locationVariance;
  locationVariance.reserve(set.getLocationCount());
  for (auto copy : set.getValuesArrays()) {
    sortIfNeeded(copy);
    auto uniqueItr = std::unique(copy.begin(), copy.end());
    auto count = distance(copy.begin(), uniqueItr);
    locationVariance.push_back(count);
  }
  assert(!locationVariance.empty());
  const auto minmaxItrPair =
      std::minmax_element(locationVariance.begin(), locationVariance.end());
  const auto minItr = minmaxItrPair.first;
  const auto maxItr = minmaxItrPair.second;
  const auto maxCount =
      std::count(locationVariance.begin(), locationVariance.end(), *maxItr);
  return maxCount > 1 ? -1 : std::distance(locationVariance.begin(), minItr);
}

int retainFirstLocation(const PatternSet &set) {
  assert(!set.isReady());
  const auto lastIndex = set.getLocationCount() - 1;
  const auto &pattern = set.getPattern();
  const auto dotPos = pattern.find_last_of('.');
  if (dotPos == std::string::npos)
    return lastIndex;
  const auto locations = getLocations(set.getLocationCount(), pattern);
  for (int i = lastIndex; i >= 0; --i)
    if (locations[i].first < dotPos)
      return i;
  return lastIndex; // might be a hidden file so taking the last
}

typedef std::function<int(const PatternSet &)> SplitIndexFunction;

inline SplitIndexFunction getSplitter(SplitIndexStrategy strategy) {
  switch (strategy) {
  case RETAIN_NONE:
    return retainNone;
  case RETAIN_FIRST_LOCATION:
    return retainFirstLocation;
  case RETAIN_LAST_LOCATION:
    return retainLastLocation;
  case RETAIN_HIGHEST_VARIANCE:
    return retainHighestVariance;
  default:
    throw std::invalid_argument("Unknown split index strategy");
  }
}

} // namespace

} // namespace details

FolderContent parse(const Configuration &config,
                    const GetNextEntryFunction &getNextEntry) {
  using namespace sequence::details;

  Trie trie;
  FolderContent result;
  Items &directories = result.directories;
  Items &files = result.files;
  { // ingest
    FilesystemEntry entry;
    while (getNextEntry(entry)) {
      if (entry.isDirectory)
        result.directories.emplace_back(STRING(entry.pFilename));
      else {
        const auto overflowed = trie.push(entry.pFilename);
        if (overflowed)
          files.emplace_back(entry.pFilename);
      }
    }
  }
  const SplitIndexFunction splitFunction = getSplitter(config.getPivotIndex);
  for (auto &item : trie.reduceToIndicedItems(splitFunction))
    files.emplace_back(std::move(item));
  if (config.mergePadding && files.size() >= 2) {
    sortIfNeeded(files, less);
    auto currentItr = files.begin();
    for (auto nextItr = currentItr + 1; nextItr != files.end(); ++nextItr) {
      const bool merged = merge(*currentItr, *nextItr);
      if (!merged) {
        ++currentItr;
        if (currentItr != nextItr) {
          *(currentItr) = std::move(*nextItr);
        }
      }
    }
    assert(currentItr != files.end());
    files.erase(++currentItr, files.end());
  }
  if (config.pack) {
    Items tmp;
    tmp.reserve(files.size());
    for (auto &item : files)
      reduceToPackedItems(item, [&tmp](Item &&i) { tmp.emplace_back(i); });
    tmp.swap(files);
  }
  if (config.bakeSingleton)
    for (auto &item : files)
      bakeSingleton(item);
  if (config.sort) {
    sortIfNeeded(directories, less);
    sortIfNeeded(files, less);
  }
  return result;
}

#ifdef _WIN64
#include "Win32FolderLister.inl"
#elif _WIN32
#include "Win32FolderLister.inl"
#elif __APPLE__
#include "LinuxFolderLister.inl"
#elif __linux
#include "LinuxFolderLister.inl"
#elif __unix // all unices not caught above
#error "Can't be compiled on 'unknown unix flavor' yet"
#elif __posix
#error "Can't be compiled on 'unknown posix flavor' yet"
#endif

FolderContent parseDir(const Configuration &configuration,
                       const CHAR *foldername) {
  Lister lister(foldername);
  auto content = parse(configuration, lister());
  content.name = foldername;
  return content;
}

} // namespace sequence
