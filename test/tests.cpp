#include <gtest/gtest.h>

#define GTEST
#include "../src/FolderParser.cpp"
#include "../src/Tools.cpp"
#undef GTEST

using sequence::Item;

using namespace std;


namespace sequence {

/**
 * Items tests
 */

TEST(Items, type) {
    Item item;
    EXPECT_EQ(Item::INVALID, item.getType());
    item.filename = "file";
    EXPECT_EQ(Item::SINGLE, item.getType());
    item.indices.push_back(2);
    EXPECT_EQ(Item::INDICED, item.getType());
    item.indices.clear();
    EXPECT_EQ(Item::SINGLE, item.getType());
    item.step = 0;
    EXPECT_EQ(Item::PACKED, item.getType());
}

TEST(Items, defaultCTor) {
    Item item;
    EXPECT_EQ(item.start, -1);
    EXPECT_EQ(item.end, -1);
    EXPECT_EQ(item.padding, -1);
    EXPECT_EQ(item.step, -1);
}

TEST(Items, moveCTor) {
    Item origin;
    origin.start = 1;
    origin.end = 2;
    origin.padding = 3;
    origin.step = 4;
    origin.filename = "filename";
    origin.indices= {1,2,3,4};
    const Item moved(std::move(origin));
    EXPECT_EQ(moved.start, 1);
    EXPECT_EQ(moved.end, 2);
    EXPECT_EQ(moved.padding, 3);
    EXPECT_EQ(moved.step, 4);
    EXPECT_EQ(moved.filename, "filename");
    EXPECT_EQ(moved.indices, VALUES( { 1, 2, 3, 4 }));
    EXPECT_TRUE(origin.filename.empty());
    EXPECT_TRUE(origin.indices.empty());
}

/**
 * Tools tests
 */

void checkInvalid(const Item& item) {
    EXPECT_EQ(item.getType(), Item::INVALID);
}

TEST(Tools, createFile) {
    // a file that looks like a pattern is invalid
    checkInvalid(createSingleFile("file#.jpg"));
    checkInvalid(createSingleFile("file@.jpg"));

    const Item item = createSingleFile("file.jpg");
    EXPECT_EQ(item.getType(), Item::SINGLE);
    EXPECT_EQ(item.filename, "file.jpg");
}

TEST(Tools, createSequence) {
    // valid sequences
    {
        // empty prefix, suffix is ok
        const Item item = createSequence("", "", 0, 0);
        EXPECT_EQ(item.getType(), Item::PACKED);
        EXPECT_EQ(item.filename, "#");
        EXPECT_EQ(item.padding, 1);
        EXPECT_EQ(item.start, 0);
        EXPECT_EQ(item.end, 0);
    }
    {
        // normal creation
        const Item item = createSequence("file-", ".png", 10, 20, 3);
        EXPECT_EQ(item.getType(), Item::PACKED);
        EXPECT_EQ(item.filename, "file-###.png");
        EXPECT_EQ(item.padding, 3);
        EXPECT_EQ(item.start, 10);
        EXPECT_EQ(item.end, 20);
    }
    // invalid if end < start
    checkInvalid(createSequence("", "", 10, 0));
    // invalid if prefix or suffix contains a # or @
    checkInvalid(createSequence("#", "", 0, 0));
    checkInvalid(createSequence("", "@", 0, 0));
    // invalid if step is 0
    checkInvalid(createSequence("", "", 0, 0, 0, 0));
}

TEST(Tools, matcherItem) {
    EXPECT_EQ(getMatcherString("@"), "#+");
    EXPECT_EQ(getMatcherString("file###.jpg"), "file###\\.jpg");
    EXPECT_EQ(getMatcherString("*#.jpg"), ".*#+\\.jpg");
    EXPECT_THROW(getMatcherString(""), std::invalid_argument);
    EXPECT_THROW(getMatcherString("missing_padding_character"), std::invalid_argument);
}

void match(const string& pattern, const std::string& prefix, const std::string& suffix, char padding = 1) {
    const auto candidate = createSequence(prefix, suffix, 0, 0, padding);
    EXPECT_TRUE(match(getMatcher(pattern), candidate)) << "matching '" << candidate.filename << "' with '" << getMatcherString(pattern) << "'";
}

TEST(Tools, match) {
    match("file-#.png", "file-", ".png");
    match("file-@.png", "file-", ".png", 3);
    match("file-#*", "file-", ".tif");
    match("*-#-*", "file-", "-.cr2");
}

TEST(Tools, filter) {
    Items items = { createSequence("file-", ".png", 1, 2), createSequence("file-", ".jpg", 1, 2) };
    const std::regex matcher = getMatcher("file-@.png");
    auto predicate = [&](const Item& item) -> bool {
        return !match(matcher, item);
    };
    items.erase(std::remove_if(items.begin(), items.end(), predicate), items.end());
    EXPECT_EQ(items.size(), 1);
}

/**
 * FolderParser tests
 */

namespace details {

TEST(Items, fromUnitFilePatternSet) {
    PatternSet set(0, "file.ext");
    ASSERT_TRUE(set.isUnitFile());
    Items items;
    set.addItems(items);
    ASSERT_EQ(Item::SINGLE, items.back().getType());
}

TEST(Items, fromRegularPatternSet) {
    PatternSet set(1, "file###.ext");
    set.addLocationValues( { 10 });
    set.addLocationValues( { 11 });
    ASSERT_TRUE(set.isReady());
    Items items;
    set.addItems(items);
    auto &item = items.back();
    ASSERT_EQ(Item::INDICED, item.getType());
    ASSERT_EQ(VALUES( { 10, 11 }), item.indices);
}

TEST(Items, fromDegeneratedPatternSet) {
    PatternSet set(2, "file###.##.ext");
    set.addLocationValues( { 0, 1 });
    set.addLocationValues( { 1, 2 });
    set.addLocationValues( { 2, 3 });
    Items items;
    set.addItems(items);
    ASSERT_EQ(3, items.size());
    EXPECT_EQ("file000.01.ext", items[0].filename);
    EXPECT_EQ("file001.02.ext", items[1].filename);
    EXPECT_EQ("file002.03.ext", items[2].filename);
}

static void checkRegular(const Item &item, index_type start, index_type end, char padding, char step) {
    EXPECT_EQ(Item::PACKED, item.getType());
    EXPECT_EQ(start, item.start);
    EXPECT_EQ(end, item.end);
    EXPECT_EQ(step, item.step);
    EXPECT_EQ(padding, item.padding);
}

TEST(Items, compactTrivial) {
    Item item;
    item.filename = "file#";
    item.indices= {0,1}; // must be sorted beforehand
    Items results;
    reduceToPackedItems(item, [&](Item&& item) {results.emplace_back(item);});
    ASSERT_EQ(1, results.size());
    checkRegular(results[0], 0, 1, 1, 1);
}

TEST(Items, compactSeveral) {
    Item item;
    item.filename = "file##";
    item.indices= {0,1,3,4,5}; // must be sorted beforehand
    Items results;
    reduceToPackedItems(item, [&](Item&& item) {results.emplace_back(item);});
    ASSERT_EQ(2, results.size());
    checkRegular(results[0], 0, 1, 2, 1);
    checkRegular(results[1], 3, 5, 2, 1);
}

TEST(Items, compactWithStep) {
    Item item;
    item.filename = "file###";
    item.indices= {0,2,4,6,8}; // must be sorted beforehand
    Items results;
    reduceToPackedItems(item, [&](Item&& item) {results.emplace_back(item);});
    ASSERT_EQ(1, results.size());
    checkRegular(results[0], 0, 8, 3, 2);
}

TEST(Items, compactSeveralWithStep) {
    Item item;
    item.filename = "file###";
    item.indices= {0,2,4,8,10,12}; // must be sorted beforehand
    Items results;
    reduceToPackedItems(item, [&](Item&& item) {results.emplace_back(item);});
    ASSERT_EQ(2, results.size());
    checkRegular(results[0], 0, 4, 3, 2);
    checkRegular(results[1], 8, 12, 3, 2);
}

TEST(Items, compactRandomGrouping) {
    Item item;
    item.filename = "file###";
    item.indices= {0,5,6,25,32}; // must be sorted beforehand
    Items results;
    reduceToPackedItems(item, [&](Item&& item) {results.emplace_back(item);});
    ASSERT_EQ(4, results.size());
    checkRegular(results[0], 0, 0, 3, 1);
    checkRegular(results[1], 5, 6, 3, 1);
    checkRegular(results[2], 25, 25, 3, 1);
    checkRegular(results[3], 32, 32, 3, 1);
}

TEST(IntegerParser, overflow) {
    IntegerParser parser;
    string ko = "123456789123456789123456798";
    for_each(ko.begin(), ko.begin() + 4, [&](CHAR c) {parser.put(c);});
    ASSERT_FALSE(parser.overflowed);
    ASSERT_EQ(1234, parser.current);
    parser.reset();
    for_each(ko.begin(), ko.end(), [&](CHAR c) {parser.put(c);});
    ASSERT_TRUE(parser.overflowed);
}

TEST(IntegerParser, overflow2) {
    IntegerParser parser;
    string ko = "5186601659";
    for_each(ko.begin(), ko.begin() + 4, [&](CHAR c) {parser.put(c);});
    ASSERT_FALSE(parser.overflowed);
    ASSERT_EQ(5186, parser.current);
    parser.reset();
    for_each(ko.begin(), ko.end(), [&](CHAR c) {parser.put(c);});
    ASSERT_TRUE(parser.overflowed);
}

TEST(Trie, simpleFileNoNumbers) {
    Trie trie;
    trie.push("simpleFileWithoutNumber");
    auto sets = trie.dataSets();
    ASSERT_EQ(1, sets.size())<< "Only one set";
    auto &pData = sets.front();
    EXPECT_TRUE(pData->getLocationValues().empty()) << "No numbers";
    EXPECT_EQ("simpleFileWithoutNumber", pData->getPattern());
    EXPECT_TRUE(pData->isUnitFile());
}

TEST(Trie, simpleFileOneNumber) {
    Trie trie;
    trie.push("toto0023");
    auto sets = trie.dataSets();
    ASSERT_EQ(1, sets.size());
    const auto &pData = sets.front();
    const auto &locations = pData->getLocationValues();
    ASSERT_EQ(1, locations.size())<< "One location";
    ASSERT_EQ(1, locations[0].size())<< "One value in first location";
    EXPECT_EQ(23, locations[0][0]) << "value is 23";
    EXPECT_EQ("toto####", pData->getPattern());
    EXPECT_FALSE(pData->isUnitFile());
}

TEST(Trie, simpleFileSeveralNumber) {
    Trie trie;
    trie.push("2-file-0254.cr2");
    auto sets = trie.dataSets();
    ASSERT_EQ(1, sets.size());
    const auto &pData = sets.front();
    const auto &locations = pData->getLocationValues();
    ASSERT_EQ(3, locations.size())<< "One location";
    ASSERT_EQ(1, locations[0].size());
    EXPECT_EQ(2, locations[0][0]);
    ASSERT_EQ(1, locations[1].size());
    EXPECT_EQ(254, locations[1][0]);
    ASSERT_EQ(1, locations[2].size());
    EXPECT_EQ(2, locations[2][0]);
    EXPECT_EQ("#-file-####.cr#", pData->getPattern());
}

TEST(Trie, problematicReduce_Flatten) {
    Trie trie;
    trie.push("file001.01.ext");
    trie.push("file001.02.ext");
    trie.push("file002.03.ext");
    auto items = trie.reduceToIndicedItems(retainNone);
    ASSERT_EQ(3, items.size());
    EXPECT_EQ("file001.01.ext", items[0].filename);
    EXPECT_EQ("file001.02.ext", items[1].filename);
    EXPECT_EQ("file002.03.ext", items[2].filename);
    for (const auto &item : items) {
        EXPECT_EQ(Item::SINGLE, item.getType());
    }
}

TEST(Trie, problematicReduce_Split) {
    Trie trie;
    trie.push("file001.01.ext");
    trie.push("file001.02.ext");
    trie.push("file002.03.ext");
    auto items = trie.reduceToIndicedItems([](const PatternSet&) {return 0;});
    ASSERT_EQ(2, items.size());
    EXPECT_EQ("file001.##.ext", items[0].filename);
    EXPECT_EQ(VALUES( { 1, 2 }), items[0].indices);
    EXPECT_EQ(Item::INDICED, items[0].getType());
    EXPECT_EQ("file002.03.ext", items[1].filename);
    EXPECT_EQ(Item::SINGLE, items[1].getType());
}

TEST(PatternSet, bakeConstantLocations) {
    PatternSet set(2, "file##.##");
    set.addLocationValues( { 5, 3 });
    EXPECT_FALSE(set.isReady());
    const auto &locations = set.getLocationValues();
    ASSERT_EQ(2, locations.size());
    ASSERT_EQ(1, locations[0].size());
    EXPECT_EQ(5, locations[0][0]);
    ASSERT_EQ(1, locations[1].size());
    EXPECT_EQ(3, locations[1][0]);
    set.bakeConstantLocations();
    EXPECT_TRUE(set.isReady());
    EXPECT_TRUE(locations.empty());
    EXPECT_EQ("file05.03", set.getPattern());
}

TEST(PatternSet, readyAfterBaking) {
    PatternSet set(2, "file##.##");
    set.addLocationValues( { 5, 3 });
    set.addLocationValues( { 10, 3 });
    set.bakeConstantLocations();
    EXPECT_TRUE(set.isReady());
    EXPECT_EQ("file##.03", set.getPattern());
    ASSERT_EQ(1, set.getLocationValues().size());
    ASSERT_EQ(2, set.getLocationValues()[0].size());
    ASSERT_EQ(5, set.getLocationValues()[0][0]);
    ASSERT_EQ(10, set.getLocationValues()[0][1]);
}

TEST(PatternSet, notReadyAfterBaking) {
    PatternSet set(2, "file##.##");
    set.addLocationValues( { 5, 3 });
    set.addLocationValues( { 1, 3 });
    set.addLocationValues( { 5, 1 });
    set.bakeConstantLocations();
    EXPECT_FALSE(set.isReady());
    EXPECT_EQ("file##.##", set.getPattern());
    auto splitted = set.split(0);
    EXPECT_EQ(2, splitted.size());
    auto & first = *splitted[0];
    EXPECT_TRUE(first.isReady());
    EXPECT_TRUE(first.isUnitFile());
    EXPECT_EQ("file01.03", first.getPattern());
    auto & second = *splitted[1];
    EXPECT_TRUE(second.isReady());
    EXPECT_FALSE(second.isUnitFile());
    EXPECT_EQ("file05.##", second.getPattern());
}

TEST(Locations, single) {
    auto locations = getLocations(1, "#");
    ASSERT_EQ(1, locations.size());
    EXPECT_EQ(Location(0, 1), locations[0]);
}

TEST(Locations, multiple) {
    auto locations = getLocations(3, "file.#.#.cr#");
    ASSERT_EQ(3, locations.size());
    EXPECT_EQ(Location(5, 6), locations[0]);
    EXPECT_EQ(Location(7, 8), locations[1]);
    EXPECT_EQ(Location(11, 12), locations[2]);
}

TEST(Locations, baking) {
    string file = "filename###.jpg";
    auto locations = getLocations(1, file);
    bake(locations[0], file, 123);
    EXPECT_EQ("filename123.jpg", file);
}

TEST(Split, retainNone) {
    PatternSet set(2, "");
    set.addLocationValues( { 0, 1 });
    set.addLocationValues( { 0, 2 });
    set.addLocationValues( { 0, 3 });
    ASSERT_EQ(-1, retainNone(set));
}

TEST(Split, retainLastLocation) {
    PatternSet set(2, "");
    set.addLocationValues( { 0, 1 });
    set.addLocationValues( { 0, 2 });
    set.addLocationValues( { 0, 3 });
    ASSERT_EQ(0, retainLastLocation(set));
}

TEST(Split, lowestVariance_1) {
    PatternSet set(2, "");
    set.addLocationValues( { 0, 1 });
    set.addLocationValues( { 0, 2 });
    set.addLocationValues( { 0, 3 });
    ASSERT_EQ(0, retainHighestVariance(set));
}

TEST(Split, lowestVariance_2) {
    PatternSet set(2, "");
    set.addLocationValues( { 1, 0 });
    set.addLocationValues( { 2, 0 });
    ASSERT_EQ(1, retainHighestVariance(set));
}

TEST(Split, lowestVariance_Ambiguous) {
    PatternSet set(3, "");
    set.addLocationValues( { 0, 1, 2 });
    set.addLocationValues( { 1, 2, 2 });
    ASSERT_EQ(-1, retainHighestVariance(set));
}

TEST(Split, lowestVariance_Bug1) {
    PatternSet set(5, "CP-sq####sq####_##-v###.####.jpg");
    for (index_type i = 1; i < 15; ++i)
        set.addLocationValues( { 88, 87, 2, 1, i });
    ASSERT_EQ(0, retainHighestVariance(set));
}

TEST(Split, lastBeforeExtension_normal) {
    PatternSet set(3, "file##-##-##.dpx");
    ASSERT_EQ(2, retainFirstLocation(set));
}

TEST(Split, lastBeforeExtension_numberInExtension) {
    PatternSet set(3, "file##-##.dp#");
    ASSERT_EQ(1, retainFirstLocation(set));
}

TEST(Split, lastBeforeExtension_hiddenFile) {
    PatternSet set(3, ".file##-##-##.dpx");
    ASSERT_EQ(2, retainFirstLocation(set))<< "returning the last in this case";
}

TEST(Split, lastBeforeExtension_hiddenFileWithExtension) {
    PatternSet set(3, ".file##-##.dp#");
    ASSERT_EQ(1, retainFirstLocation(set))<< "returning the last in this case";
}

TEST(Items, mergeDecade) {
    Item a, b;
    a.filename = "file##.ext";
    b.filename = "file#.ext";
    a.indices = {11,21};
    b.indices = {5,6};
    EXPECT_TRUE(merge(a, b));
    EXPECT_EQ(VALUES({5, 6, 11, 21}), a.indices);
    EXPECT_EQ(0, a.padding);
    EXPECT_EQ("file#.ext", a.filename);
}

TEST(Items, mergeThousands) {
    Item a, b;
    a.filename = "file##.ext";
    b.filename = "file####.ext";
    a.indices = {11, 21};
    b.indices = {1234, 1235};
    EXPECT_TRUE(merge(a, b));
    EXPECT_EQ(VALUES({11, 21, 1234, 1235}), a.indices);
    EXPECT_EQ(0, a.padding);
    EXPECT_EQ("file#.ext", a.filename);
}

TEST(Items, mergeMismatch) {
    Item a, b, c;
    a.filename = "file#.ext";
    b.filename = "file##xyz.ext";
    c.filename = "filexyz##.ext";
    a.indices = {1, 2};
    b.indices = {10};
    c.indices = {20};
    EXPECT_FALSE(merge(a, b));
    EXPECT_FALSE(merge(a, c));
}

TEST(Items, mergeSharedIndices) {
    Item a, b;
    a.filename = "file###.ext";
    b.filename = "file#.ext";
    a.indices = {1, 2};
    b.indices = {1, 5};
    EXPECT_FALSE(merge(a, b));
}

struct FileProvider {
    const vector<STRING> filenames;
    vector<STRING>::const_iterator current;
    const vector<STRING>::const_iterator end;
    FileProvider(const CHAR *filename) :
                    filenames( { STRING(filename) }), current(filenames.begin()), end(filenames.end()) {
    }
    FileProvider(vector<STRING> &&files) :
                    filenames(std::move(files)), current(filenames.begin()), end(filenames.end()) {
    }
    bool operator()(FilesystemEntry &entry) {
        if (current == end)
            return false;
        entry.pFilename = current->c_str();
        entry.isDirectory = false;
        ++current;
        return true;
    }
};

TEST(Correctness, integerOverflow) {
    const char * pFilename = "5186601659_3b0ebecbb3_o.jpg";
    FileProvider provider(pFilename);
    Configuration conf;
    conf.getPivotIndex = RETAIN_NONE;
    conf.bakeSingleton = true;
    auto result = parse(conf, provider);
    EXPECT_EQ(1, result.files.size());
    const auto & file = result.files[0];
    EXPECT_EQ(pFilename, file.filename);
}

static Configuration make_conf() {
    Configuration configuration;
    configuration.getPivotIndex = RETAIN_NONE;
    configuration.mergePadding = true;
    configuration.pack = true;
    configuration.bakeSingleton = false;
    configuration.sort = true;
    return configuration;
}

TEST(Performance, 200_000_files) {
    const size_t COUNT = 32;
    char str[COUNT];
    size_t i = 0;
    auto nextEntry = [&](FilesystemEntry&entry) -> bool {
        if(i++>=200000)
        return false;
        snprintf(str, COUNT, "filename.%ld.ext", i);
        entry.isDirectory = false;
        entry.pFilename = str;
        return true;
    };
    Configuration configuration = make_conf();
    auto result = parse(configuration, nextEntry);
    EXPECT_TRUE(result.directories.empty());
    ASSERT_EQ(1, result.files.size());
    const auto &file = result.files[0];
    EXPECT_EQ("filename.#.ext", file.filename);
    checkRegular(file, 1, 200000, 0, 1);
}

TEST(Performance, 200_000_files_with_step_of_3) {
    const size_t COUNT = 32;
    char str[COUNT];
    size_t i = 0;
    auto nextEntry = [&](FilesystemEntry&entry) -> bool {
        if(i>=200000)
        return false;
        snprintf(str, COUNT, "filename.%ld.ext", i);
        entry.isDirectory = false;
        entry.pFilename = str;
        i+=3;
        return true;
    };
    Configuration configuration = make_conf();
    auto result = parse(configuration, nextEntry);
    EXPECT_TRUE(result.directories.empty());
    ASSERT_EQ(1, result.files.size());
    const auto &file = result.files[0];
    EXPECT_EQ("filename.#.ext", file.filename);
    checkRegular(file, 0, 199998, 0, 3);
}

TEST(Correctness, bakeSingleton) {
    const char * pFilename = "xmode2";
    FileProvider provider(pFilename);
    Configuration conf;
    conf.bakeSingleton = true;
    conf.pack = true;
    auto result = parse(conf, provider);
    EXPECT_EQ(1, result.files.size());
    const auto & file = result.files[0];
    EXPECT_EQ(Item::SINGLE, file.getType());
}

TEST(Correctness, merge) {
    FileProvider provider({"file1.ext", "file10.ext", "file10xyz.ext", "file100.ext"});
    Configuration conf;
    conf.mergePadding = true;
    conf.sort = true;
    conf.bakeSingleton = true;
    auto result = parse(conf, provider);
    ASSERT_EQ(2, result.files.size());
    const auto & first = result.files[0];
    EXPECT_EQ(Item::INDICED, first.getType());
    EXPECT_EQ(VALUES({1,10,100}), first.indices);
    EXPECT_EQ("file#.ext", first.filename);
    const auto & second = result.files[1];
    EXPECT_EQ(Item::SINGLE, second.getType());
    EXPECT_EQ("file10xyz.ext", second.filename);
}

TEST(Correctness, merge2) {
    FileProvider provider( { ".directory", "f0101.ext", "f0100.ext", "f0098.ext", "f0099.ext" });
    Configuration conf;
    conf.mergePadding = true;
    conf.sort = true;
    conf.bakeSingleton = true;
    conf.pack = true;
    auto result = parse(conf, provider);
    ASSERT_EQ(2, result.files.size());
    const auto & first = result.files.at(0);
    EXPECT_EQ(Item::SINGLE, first.getType());
    EXPECT_EQ(".directory", first.filename);
    const auto & second = result.files.at(1);
    EXPECT_EQ(Item::PACKED, second.getType());
    EXPECT_TRUE(second.indices.empty());
    EXPECT_EQ(second.start, 98);
    EXPECT_EQ(second.end, 101);
    EXPECT_EQ(second.step, 1);
    EXPECT_EQ("f####.ext", second.filename);
}

} // namespace details

} // namespace sequence

namespace std {

ostream& operator<<(ostream& s, const sequence::VALUES& values) {
    s << "[";
    auto i = values.begin();
    if(i!=values.end()) {
        s << *i;
        ++i;
        for(; i != values.end(); ++i) {
            s << ", " << *i;
        }
    }
    s << "]";
    return s;
}

ostream& operator<<(ostream& s, const sequence::Item& item) {
    switch (item.getType()) {
    case sequence::Item::INVALID:
        s << "invalid";
        break;
    case sequence::Item::SINGLE:
        s << item.filename;
        break;
    case sequence::Item::INDICED:
        s << item.filename << " (" << item.indices <<  ")";
        break;
    case sequence::Item::PACKED:
        s << item.filename << " (" << item.start << "-" << item.end << "+" << int(item.step) <<  ")";
        break;
    }
    return s;
}

ostream& operator<<(ostream& s, const sequence::Items& items) {
    s << "[";
    auto i = items.begin();
    if(i!=items.end()) {
        s << *i;
        ++i;
        for(; i != items.end(); ++i) {
            s << ", " << *i;
        }
    }
    s << "]";
    return s;
}

ostream& operator<<(ostream& s, const sequence::FolderContent& c) {
    return s << "files: " << c.files << ", directories: " << c.directories << ", name: " << c.name;
}
}

namespace additionalTests {
struct Lister {
private:
    vector<string> m_files;
    bool m_noMoreFile;
    int m_fileIndex;
public:
    Lister(const vector<string> &files)
        :m_files(files), m_fileIndex(0)
    {
        m_noMoreFile = m_files.empty();
    }

    function<bool(sequence::FilesystemEntry&)> operator()() {
        return [&](sequence::FilesystemEntry &entry) -> bool {
            if(m_noMoreFile)
                return false;
            entry.isDirectory = false;
            entry.pFilename = m_files[m_fileIndex].c_str();
            //find index
            ++m_fileIndex;
            if ( m_fileIndex >= m_files.size() )
                m_noMoreFile = true;
            return true;
        };
    }

    ~Lister() {
    }
};

/**
 * @brief The SequenceUtilsTester class is a basic test fixture
 */
class SequenceParserTest : public testing::Test {
public:
    SequenceParserTest() {

    }

    // virtual void SetUp() will be called before each test is run.  You
    // should define it if you need to initialize the varaibles.
    // Otherwise, this can be skipped.
    virtual void SetUp() {
    }

    // virtual void TearDown() will be called after each test is run.
    // You should define it if there is cleanup work to do.  Otherwise,
    // you don't have to provide it.
    //
    virtual void TearDown() {
    }

    static sequence::Configuration getParserConf(bool useSequences) {
        using namespace sequence;
        Configuration conf;
        if (useSequences)
            conf.getPivotIndex = RETAIN_HIGHEST_VARIANCE;
        else
            conf.getPivotIndex = RETAIN_NONE;
        conf.sort = false;
        conf.bakeSingleton = useSequences;
        conf.mergePadding = useSequences;
        conf.pack = useSequences;
        return conf;
    }
};

/*
  Tests for sequence parser
*/

namespace {
template<typename T> T limit_max(const T&) {
    return numeric_limits<T>::max();
}

Item createIndices(const std::string & prefix, const std::string & postfix, int start, int end, int padding){
    Item item = sequence::createSequence(prefix,postfix,start,end,padding);
    while(start<=end){
        item.indices.push_back(start);
        ++start;
    }
    return item;
}
}



TEST_F(SequenceParserTest, merge)
{
    Item item0 = createIndices("file",".ext",0,9,2);
    Item item1 = createIndices("file",".ext",0,9,1);
    ASSERT_EQ(false, sequence::details::merge(item0, item1));
    Item item2 = createIndices("file",".ext",10,99, 2);
    Item item3 = createIndices("file",".ext",101,999, 3);
    ASSERT_EQ(true, sequence::details::merge(item1, item2));
    ASSERT_EQ(true, sequence::details::merge(item1, item3));
}


TEST_F(SequenceParserTest, singleIndex)
{
    vector<string> files;
    files.push_back("file1.ext");

    Lister lister(files);
    auto content = sequence::parse(getParserConf(true), lister());

    ASSERT_EQ(1, content.files.size());
    const auto& item = content.files.front();
    ASSERT_EQ(sequence::Item::Type::SINGLE, item.getType());
    ASSERT_EQ(files.front(), item.filename);
}

TEST_F(SequenceParserTest, bigStep)
{
    vector<string> files;
    files.push_back("sintel_trailer_2k_0368.png");
    files.push_back("sintel_trailer_2k_1071.png");

    Lister lister(files);
    auto content = sequence::parse(getParserConf(true), lister());

    auto expectedStep = 1071 - 368;
    const auto& item1 = content.files.front();
    ASSERT_GE(content.files.size(), 1);

    if(expectedStep > limit_max(item1.step)) {
        // if step field can't hold size it should be two items

        ASSERT_EQ(2, content.files.size());
        ASSERT_EQ(sequence::Item::Type::SINGLE, item1.getType());
        ASSERT_EQ(files[0], item1.filename);

        const auto& item2 = content.files[1];
        ASSERT_EQ(sequence::Item::Type::SINGLE, item2.getType());
        ASSERT_EQ(files[1], item2.filename);
    } else {
        ASSERT_EQ(1, content.files.size());
        ASSERT_EQ(sequence::Item::Type::PACKED, item1.getType());
        ASSERT_EQ(368, item1.start);
        ASSERT_EQ(1071, item1.end);
        ASSERT_EQ(expectedStep, item1.step);
    }
}

TEST_F(SequenceParserTest, test8_10_16) {
    vector<int> indeces{8,10,16};

    vector<string> files;
    for(const auto& i : indeces) {
        stringstream stream;
        stream << "file" << i << ".ext";
        files.push_back(stream.str());
    }

    Lister lister(files);
    ASSERT_PRED1([&](sequence::FolderContent content)->bool {
        if(content.files.size() == indeces.size()) {
            // single file items acceptible
            return std::all_of( content.files.begin(), content.files.end(),
                                [](const sequence::Item& item) -> bool{
                return item.getType() == sequence::Item::Type::SINGLE;
            });
        }
        else if(content.files.size() == indeces.size() - 1) {
            // any pair is acceptible
            if(content.files[0].getType() == sequence::Item::Type::PACKED) {
                const auto& indexed = content.files[0];
                const auto& single = content.files[1];

                if(indexed.start == indeces[0] && indexed.end == indeces[2]) {
                    return single.filename == files[1] && (indeces[2]-indeces[0]) == indexed.step;
                } else if(indexed.start == indeces[1] && indexed.end == indeces[2]) {
                    return single.filename == files[0] && (indeces[2]-indeces[1]) == indexed.step;
                } else if(indexed.start == indeces[0] && indexed.end == indeces[1]) {
                    return single.filename == files[2] && (indeces[1]-indeces[0]) == indexed.step;
                }
            }
        }
        return false;
    }, sequence::parse(getParserConf(true), lister()));
}

TEST_F(SequenceParserTest, disconnectedSequence) {
    vector<int> indeces{2,3,4, 10,11,12};
    const auto width = 2;
    vector<string> files;
    for(const auto& i : indeces) {
        stringstream stream;
        stream << "file";
        stream.width(width);
        stream.fill('0');
        stream << right << i;
        stream << ".ext";
        files.push_back(stream.str());
    }


    Lister lister(files);
    auto content = sequence::parse(getParserConf(true), lister());
    ASSERT_EQ(2, content.files.size());

    ASSERT_EQ(2, content.files[0].start);
    ASSERT_EQ(4, content.files[0].end);
    ASSERT_EQ(1, content.files[0].step);
    ASSERT_EQ(2, content.files[0].padding);
    ASSERT_EQ(sequence::Item::Type::PACKED, content.files[0].getType());

    ASSERT_EQ(10, content.files[1].start);
    ASSERT_EQ(12, content.files[1].end);
    ASSERT_EQ(1,  content.files[1].step);
    ASSERT_PRED1([](int i)->bool{ return i == 0 || i == 2; },  content.files[1].padding);
    ASSERT_EQ(sequence::Item::Type::PACKED, content.files[1].getType());
}


TEST_F(SequenceParserTest, disconnectedSequence2) {
    vector<int> indeces{2,3,4, 100,101,102};
    const auto width = 2;
    vector<string> files;
    for(const auto& i : indeces) {
        stringstream stream;
        stream << "file";
        stream.width(width);
        stream.fill('0');
        stream << right << i;
        stream << ".ext";
        files.push_back(stream.str());
    }


    Lister lister(files);
    auto content = sequence::parse(getParserConf(true), lister());
    ASSERT_EQ(2, content.files.size());

    ASSERT_EQ(2, content.files[0].start);
    ASSERT_EQ(4, content.files[0].end);
    ASSERT_EQ(1, content.files[0].step);
    ASSERT_EQ(2, content.files[0].padding);
    ASSERT_EQ(sequence::Item::Type::PACKED, content.files[0].getType());

    ASSERT_EQ(100, content.files[1].start);
    ASSERT_EQ(102, content.files[1].end);
    ASSERT_EQ(1,  content.files[1].step);
    ASSERT_PRED1([](int i)->bool{ return i == 0 || i == 3; },  content.files[1].padding);
    ASSERT_EQ(sequence::Item::Type::PACKED, content.files[1].getType());
}
}
