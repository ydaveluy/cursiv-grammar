#pragma once

#include <string_view>
#include "cursiv/core/grammar/Conversion.hpp"

namespace r {
inline bool valid_escapes(std::string_view text, bool symbol = false) {
  bool byteEscape = false, unicodeEscape = false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\0') return false;
    if (text[i] != '\\') continue;
    if (++i == text.size()) return false;
    const char e = text[i];
    if (std::string_view("abfnrtv\\'\"`\r\n").find(e) != std::string_view::npos) continue;
    const bool octal = e >= '0' && e <= '7';
    const bool unicode = e == 'u' || e == 'U';
    if (!octal && e != 'x' && !unicode) return false;
    if (unicode && symbol) return false;
    unicodeEscape |= unicode; byteEscape |= !unicode;
    if (unicodeEscape && byteEscape) return false;
    unsigned value = octal ? e - '0' : 0, count = octal ? 1 : 0;
    const unsigned limit = octal ? 3 : e == 'x' ? 2 : e == 'u' ? 4 : 8;
    const bool braced = unicode && i + 1 < text.size() && text[i + 1] == '{';
    if (braced) ++i;
    while (i + 1 < text.size() && count < limit) {
      const auto c = text[i + 1];
      const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
          c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
      if (digit >= (octal ? 8u : 16u)) break;
      value = value * (octal ? 8 : 16) + digit; ++count; ++i;
    }
    if (!count || value == 0 || value > (unicode ? 0x10ffffu : 255u)) return false;
    if (braced && (++i == text.size() || text[i] != '}')) return false;
  }
  return true;
}
inline constexpr auto unquote = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto body = text.substr(1, text.size() - 2);
  const bool symbol = text.front() == '`';
  if ((symbol && body.empty()) || !valid_escapes(body, symbol))
    return Result::err("Invalid R quoted string or identifier escape.");
  return Result::ok(body); // Preserve source escapes for later interpretation.
};
}  // namespace r
