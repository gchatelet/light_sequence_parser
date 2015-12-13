#include "sequence/details/StringView.hpp"
#include "sequence/details/Hash.hpp"

#include "gtest/gtest.h"

TEST(CStringView, default_ctor) {
  CStringView a;
  EXPECT_EQ(a.ptr(), nullptr);
  EXPECT_EQ(a.size(), 0);
  EXPECT_TRUE(a.empty());
}

TEST(CStringView, empty_string_ctor) {
  CStringView a("");
  EXPECT_NE(a.ptr(), nullptr);
  EXPECT_EQ(a.size(), 0);
  EXPECT_TRUE(a.empty());
}

TEST(CStringView, non_empty_string_ctor) {
  CStringView a("1");
  EXPECT_NE(a.ptr(), nullptr);
  EXPECT_EQ(a.size(), 1);
  EXPECT_FALSE(a.empty());
  EXPECT_EQ(a[0], '1');
}

TEST(CStringView, empty_std_string_ctor) {
  const std::string from;
  CStringView a(from);
  EXPECT_NE(a.ptr(), nullptr);
  EXPECT_EQ(a.size(), 0);
  EXPECT_TRUE(a.empty());
}

TEST(CStringView, non_empty_std_string_ctor) {
  const std::string from("1");
  CStringView a(from);
  EXPECT_NE(a.ptr(), nullptr);
  EXPECT_EQ(a.size(), 1);
  EXPECT_FALSE(a.empty());
  EXPECT_EQ(a[0], '1');
}

TEST(CStringView, default_copy_ctor) {
  CStringView a;
  CStringView b = a;
  EXPECT_EQ(b.ptr(), nullptr);
  EXPECT_EQ(b.size(), 0);
  EXPECT_TRUE(b.empty());
}

TEST(CStringView, empty_copy_ctor) {
  CStringView a("");
  CStringView b = a;
  EXPECT_NE(b.ptr(), nullptr);
  EXPECT_EQ(b.size(), 0);
  EXPECT_TRUE(b.empty());
}

TEST(CStringView, non_empty_copy_ctor) {
  CStringView a("1");
  CStringView b = a;
  EXPECT_NE(b.ptr(), nullptr);
  EXPECT_EQ(b.size(), 1);
  EXPECT_FALSE(b.empty());
  EXPECT_EQ(b[0], '1');
}

TEST(CStringView, equals) {
  EXPECT_EQ(CStringView(), CStringView());
  EXPECT_EQ(CStringView(""), CStringView(""));
  EXPECT_EQ(CStringView("1"), CStringView("1"));
  EXPECT_EQ(CStringView(""), "");
  EXPECT_EQ(CStringView("1"), "1");

  EXPECT_NE(CStringView("1"), CStringView(""));
  EXPECT_NE(CStringView("1"), CStringView("0"));
  EXPECT_NE(CStringView("1"), "");
  EXPECT_NE(CStringView("1"), "0");
}

TEST(CStringView, less) {
  EXPECT_FALSE(CStringView() < CStringView());
  EXPECT_FALSE(CStringView("a") < CStringView("a"));
  EXPECT_TRUE(CStringView("a") < CStringView("b"));
  EXPECT_TRUE(CStringView("a") < CStringView("ab"));
  EXPECT_FALSE(CStringView("ab") < CStringView("ab"));
}

TEST(CStringView, indexOfChar) {
  EXPECT_EQ(CStringView().indexOf('0'), CStringView::npos);
  EXPECT_EQ(CStringView("").indexOf('0'), CStringView::npos);
  EXPECT_EQ(CStringView("1").indexOf('0'), CStringView::npos);
  EXPECT_EQ(CStringView("0").indexOf('0'), 0);
  EXPECT_EQ(CStringView("10").indexOf('0'), 1);
  EXPECT_EQ(CStringView("00").indexOf('0'), 0);
}

TEST(CStringView, lastIndexOfChar) {
  EXPECT_EQ(CStringView().lastIndexOf('0'), CStringView::npos);
  EXPECT_EQ(CStringView("").lastIndexOf('0'), CStringView::npos);
  EXPECT_EQ(CStringView("1").lastIndexOf('0'), CStringView::npos);
  EXPECT_EQ(CStringView("0").lastIndexOf('0'), 0);
  EXPECT_EQ(CStringView("10").lastIndexOf('0'), 1);
  EXPECT_EQ(CStringView("00").lastIndexOf('0'), 1);
}

TEST(CStringView, containsChar) {
  EXPECT_FALSE(CStringView().contains('0'));
  EXPECT_FALSE(CStringView("").contains('0'));
  EXPECT_FALSE(CStringView("1").contains('0'));
  EXPECT_TRUE(CStringView("0").contains('0'));
  EXPECT_TRUE(CStringView("10").contains('0'));
}

TEST(CStringView, indexOfString) {
  EXPECT_EQ(CStringView().indexOf(CStringView()), CStringView::npos);
  EXPECT_EQ(CStringView().indexOf(CStringView("a")), CStringView::npos);
  EXPECT_EQ(CStringView("").indexOf(CStringView("")), CStringView::npos);
  EXPECT_EQ(CStringView("abc").indexOf(CStringView("a")), 0);
  EXPECT_EQ(CStringView("abc").indexOf(CStringView("b")), 1);
  EXPECT_EQ(CStringView("abc").indexOf(CStringView("bc")), 1);
}

TEST(CStringView, containsString) {
  EXPECT_FALSE(CStringView().contains("0"));
  EXPECT_FALSE(CStringView("").contains("0"));
  EXPECT_FALSE(CStringView("1").contains("0"));
  EXPECT_TRUE(CStringView("0").contains("0"));
  EXPECT_TRUE(CStringView("10").contains("0"));
}

TEST(CStringView, substr) {
  EXPECT_EQ(CStringView("abc").substr(), "abc");
  EXPECT_EQ(CStringView("abc").substr(1), "bc");
  EXPECT_EQ(CStringView("abc").substr(0, 1), "a");
  EXPECT_EQ(CStringView("abc").substr(0, 10), "abc");
}

TEST(CStringView, toString) {
  EXPECT_EQ(CStringView().toString(), "");
  EXPECT_EQ(CStringView("").toString(), "");
  EXPECT_EQ(CStringView("a").toString(), "a");
}

TEST(CStringView, empty_iteration) {
  CStringView str;
  EXPECT_EQ(std::distance(str.begin(), str.end()), 0);
}

TEST(CStringView, iteration) {
  CStringView str("abc");
  EXPECT_EQ(std::distance(str.begin(), str.end()), 3);
}

TEST(CStringView, hash_equality) {
  EXPECT_EQ(hash(""), hash(""));
  EXPECT_EQ(hash("1"), hash("1"));
  EXPECT_NE(hash(""), hash("1"));
  EXPECT_EQ(hash(1), hash(1));
  EXPECT_NE(hash(1), hash(2));
}

TEST(StringView, modification) {
  std::string original("a");
  StringView str(original);
  EXPECT_EQ(str, "a");
  str[0] = 'b';
  EXPECT_EQ(str, "b");
  EXPECT_EQ(original, "b");
}
