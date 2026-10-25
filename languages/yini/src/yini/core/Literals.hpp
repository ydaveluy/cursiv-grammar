#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace yini {
struct DecodedString {
  std::string value;
  std::string error;
};
DecodedString decode_string(std::string_view literal);
std::string key_value(std::string_view literal);
bool valid_key(std::string_view literal);
bool valid_number(std::string_view literal);
std::size_t section_depth(std::string_view marker);
}  // namespace yini
