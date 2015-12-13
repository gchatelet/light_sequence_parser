#include <sequence/details/StringUtils.hpp>

std::string concat(CStringView a) { return a.toString(); }

std::string concat(CStringView a, CStringView b) {
  std::string result;
  result.reserve(a.size() + b.size());
  result.append(a.ptr(), a.size());
  result.append(b.ptr(), b.size());
  return result;
}

std::string concat(CStringView a, CStringView b, CStringView c) {
  std::string result;
  result.reserve(a.size() + b.size() + c.size());
  result.append(a.ptr(), a.size());
  result.append(b.ptr(), b.size());
  result.append(c.ptr(), c.size());
  return result;
}

std::string concat(CStringView a, CStringView b, CStringView c, CStringView d) {
  std::string result;
  result.reserve(a.size() + b.size() + c.size() + d.size());
  result.append(a.ptr(), a.size());
  result.append(b.ptr(), b.size());
  result.append(c.ptr(), c.size());
  result.append(d.ptr(), d.size());
  return result;
}
