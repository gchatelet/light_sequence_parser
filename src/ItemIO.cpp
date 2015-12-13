#include "sequence/ItemIO.hpp"

#include <cassert>

namespace sequence {
const char *getTypeString(Item::Type type) {
  switch (type) {
  case Item::INVALID:
    return "invalid";
  case Item::SINGLE:
    return "single";
  case Item::INDICED:
    return "indiced";
  case Item::PACKED:
    return "packed";
  }
  assert(false);
  return "error";
}

} // namespace sequence

std::ostream &operator<<(std::ostream &stream, const sequence::Item &item) {
  switch (item.getType()) {
  case sequence::Item::SINGLE:
    stream << item.filename;
    break;
  case sequence::Item::INVALID:
    stream << sequence::Item::INVALID;
    break;
  case sequence::Item::INDICED:
    stream << item.filename << " (" << item.indices.size() << ")"
           << (int)item.padding;
    break;
  case sequence::Item::PACKED:
    if (item.step == 1) {
      stream << item.filename << " [" << item.start << ":" << item.end << "] #"
             << (int)item.padding;
    } else {
      stream << item.filename << " [" << item.start << ":" << item.end << "]/"
             << (int)item.step << " #" << (int)item.padding;
    }
    break;
  }
  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         const sequence::Item::Type type) {
  return stream << sequence::getTypeString(type);
}
