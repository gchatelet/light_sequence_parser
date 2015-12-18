#pragma once

#include <sequence/details/StringView.hpp>

uint32_t hash(CStringView view, uint32_t seed = 0);
uint32_t hash(uint32_t);
