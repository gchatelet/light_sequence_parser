#pragma once

#include <ostream>

#include <sequence/Item.hpp>

namespace sequence {

const char *getTypeString(sequence::Item::Type type);

} // namespace sequence

std::ostream &operator<<(std::ostream &stream, const sequence::Item &item);
std::ostream &operator<<(std::ostream &stream, const sequence::Item::Type type);
