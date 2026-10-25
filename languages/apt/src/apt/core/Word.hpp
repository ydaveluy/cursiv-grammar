#pragma once
#include <string>
#include <string_view>

namespace apt {
inline std::string word_value(std::string_view source) {
  auto hex = [](unsigned char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  std::string value;
  for (std::size_t i = 0; i < source.size(); ++i) {
    if (source[i] == '%' && i + 2 < source.size()) {
      const int a = hex(source[i + 1]), b = hex(source[i + 2]);
      if (a >= 0 && b >= 0) {
        value += static_cast<char>((a << 4) | b);
        i += 2;
        continue;
      }
    }
    if (source[i] != '"') value += source[i];
  }
  return value;
}
}  // namespace apt
