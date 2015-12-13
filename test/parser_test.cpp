#include <sequence/Parser.hpp>

#include <utility>

#include <gtest/gtest.h>

#include <sequence/Tools.hpp>
#include <sequence/ItemIO.hpp>

namespace sequence {

struct StringFileLister {
  StringFileLister(std::initializer_list<std::string> files) : files_(files) {
    std::reverse(files_.begin(), files_.end());
  }

  GetNextEntryFunction operator()() {
    return [&](FilesystemEntry &entry) -> bool {
      if (!first_) {
        files_.pop_back();
      }
      if (files_.empty()) {
        return false;
      }
      entry.filename = files_.back();
      entry.isDirectory = false;
      first_ = false;
      return true;
    };
  }

private:
  std::vector<std::string> files_;
  bool first_ = true;
};

TEST(Parser, singleFile) {
  StringFileLister lister({"/path/file"});
  const auto content = parse(Configuration(), lister());
  EXPECT_EQ(Items({createSingleFile("/path/file")}), content.files);
}

TEST(Parser, simpleSequence) {
  StringFileLister lister({"/path/f1.jpg", "/path/f2.jpg"});
  const auto content = parse(Configuration(), lister());
  EXPECT_EQ(Items({createSequence("/path/f#.jpg", {1, 2})}), content.files);
}

TEST(Parser, simpleSequencePacked) {
  StringFileLister lister({"/path/f1.jpg", "/path/f2.jpg", "/path/f3.jpg"});
  Configuration configuration;
  configuration.pack = true;
  const auto content = parse(configuration, lister());
  EXPECT_EQ(Items({createSequence("/path/f#.jpg", 1, 3)}), content.files);
}

TEST(Parser, bigStep) {
  StringFileLister lister(
      {"sintel_trailer_2k_0368.png", "sintel_trailer_2k_1071.png"});
  const auto content = parse(Configuration(), lister());
  EXPECT_EQ(Items({createSequence("sintel_trailer_2k_####.png", {368, 1071})}),
            content.files);
}

TEST(Parser, noStep) {
  StringFileLister lister({"file8.ext", "file10.ext", "file16.ext"});
  const auto content = parse(Configuration(), lister());
  EXPECT_EQ(Items({createSequence("file##.ext", {10, 16}),
                   createSingleFile("file8.ext")}),
            content.files);
}

TEST(Parser, disconnected) {
  StringFileLister lister({"file02.ext", "file03.ext", "file04.ext",
                           "file10.ext", "file11.ext", "file12.ext"});
  Configuration configuration;
  configuration.pack = true;
  configuration.sort = true;
  const auto content = parse(configuration, lister());
  EXPECT_EQ(Items({createSequence("file##.ext", 2, 4),
                   createSequence("file##.ext", 10, 12)}),
            content.files);
}

TEST(Parser, disconnected2) {
  StringFileLister lister({"file02.ext", "file03.ext", "file04.ext",
                           "file100.ext", "file101.ext", "file102.ext"});
  Configuration configuration;
  configuration.pack = true;
  configuration.sort = true;
  const auto content = parse(configuration, lister());
  EXPECT_EQ(Items({
                createSequence("file###.ext", 100, 102),
                createSequence("file##.ext", 2, 4),
            }),
            content.files);
}

TEST(Parser, merge) {
  StringFileLister lister({"file97.ext", "file98.ext", "file99.ext",
                           "file100.ext", "file101.ext", "file102.ext"});
  Configuration configuration;
  configuration.pack = true;
  configuration.mergePadding = true;
  const auto content = parse(configuration, lister());
  EXPECT_EQ(Items({createSequence("file#.ext", 97, 102)}), content.files);
}

} // namespace sequence
