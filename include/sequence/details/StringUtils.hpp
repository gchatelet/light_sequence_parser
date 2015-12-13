#pragma once

#include <string>

#include <sequence/details/StringView.hpp>

std::string concat(CStringView a);
std::string concat(CStringView a, CStringView b);
std::string concat(CStringView a, CStringView b, CStringView c);
std::string concat(CStringView a, CStringView b, CStringView c, CStringView d);
