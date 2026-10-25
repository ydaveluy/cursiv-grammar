#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "cursiv/core/grammar/unicode/UnicodeRanges.hpp"

namespace fen {
inline std::optional<std::string> counter_value(std::string_view text) {
  bool negative = false;
  if (text.starts_with('+') || text.starts_with('-')) {
    negative = text.front() == '-';
    text.remove_prefix(1);
  }
  std::string result;
  bool digit = false;
  for (std::size_t i = 0; i < text.size();) {
    const auto b = static_cast<unsigned char>(text[i++]);
    if (b == '_') {
      if (!digit) return {};
      digit = false;
      continue;
    }
    char32_t cp = b;
    if (b >= 0x80) {
      if (b > 0xf4) return {};
      const unsigned more = b >= 0xf0 ? 3 : b >= 0xe0 ? 2 : b >= 0xc2 ? 1 : 4;
      if (more > 3 || i + more > text.size()) return {};
      cp &= (1u << (6 - more)) - 1;
      for (unsigned k = 0; k < more; ++k) {
        const auto c = static_cast<unsigned char>(text[i++]);
        if ((c & 0xc0) != 0x80) return {};
        cp = (cp << 6) | (c & 0x3f);
      }
      if ((more == 1 && cp < 0x80) || (more == 2 && cp < 0x800) ||
          (more == 3 && cp < 0x10000) || cp > 0x10ffff ||
          (cp >= 0xd800 && cp <= 0xdfff))
        return {};
    }
    int value = -1;
    if (cp >= '0' && cp <= '9')
      value = cp - '0';
    else
      for (const auto range : cursiv::unicode::kNd)
        if (cp >= range.lo && cp <= range.hi) {
          value = (cp - range.lo) % 10;
          break;
        }
    if (value < 0) return {};
    result += char('0' + value);
    digit = true;
  }
  if (!digit) return {};
  const auto start = result.find_first_not_of('0');
  if (start == result.npos) return std::string("0");
  if (negative) return {};
  result.erase(0, start);
  return result;
}
}  // namespace fen
