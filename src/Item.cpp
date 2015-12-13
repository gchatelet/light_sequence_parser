#include <sequence/Item.hpp>

#include <tuple>

namespace sequence {
Item::Item(CStringView filename) : filename(filename.toString()) {}
Item::Item(CStringView filename, Indices &&indices)
    : filename(filename.toString()), indices(std::move(indices)) {}

Item::Type Item::getType() const {
  if (filename.empty())
    return INVALID;
  if (!indices.empty())
    return INDICED;
  if (step == -1)
    return SINGLE;
  return PACKED;
}

bool Item::operator<(const Item &o) const {
  const auto type = getType();
  const auto otherType = o.getType();
  if (type == otherType) {
    switch (type) {
    case SINGLE:
      return filename < o.filename;
    case INDICED:
      return std::tie(filename, indices) < std::tie(o.filename, o.indices);
    case PACKED:
      return std::tie(filename, start, end, padding, step) <
             std::tie(o.filename, o.start, o.end, o.padding, o.step);
    default:
    case INVALID:
      return false;
    }
  }
  return type < otherType;
}

bool Item::operator==(const Item &o) const {
  const auto type = getType();
  if (type == o.getType()) {
    switch (type) {
    case SINGLE:
      return filename == o.filename;
    case INDICED:
      return std::tie(filename, indices) == std::tie(o.filename, o.indices);
    case PACKED:
      return std::tie(filename, start, end, padding, step) ==
             std::tie(o.filename, o.start, o.end, o.padding, o.step);
    default:
    case INVALID:
      return true;
    }
  }
  return false;
}

} // namespace sequence
