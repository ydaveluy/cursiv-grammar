#pragma once

#include <string_view>

#include "cursiv/core/grammar/Conversion.hpp"

namespace scss {

// Dart Sass expectIdentifier("optional") compares decoded CSS characters.
// Keep this conversion local to the suffix; selector names stay source text.
inline constexpr auto extend_optional = [](std::string_view text) {
  using Result = cursiv::ConversionResult<bool>;
  const auto invalid = [] { return Result::err("Expected !optional."); };
  if (text.empty() || text.front() != '!') return invalid();
  std::size_t offset = 1;
  for (const char expected : std::string_view("optional")) {
    if (offset == text.size()) return invalid();
    unsigned value = static_cast<unsigned char>(text[offset++]);
    if (value == '\\') {
      if (offset == text.size()) return invalid();
      value = 0;
      unsigned digits = 0;
      while (offset < text.size() && digits < 6) {
        const auto ch = static_cast<unsigned char>(text[offset]);
        const unsigned digit = ch >= '0' && ch <= '9' ? ch - '0' :
            ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 :
            ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : 16;
        if (digit == 16) break;
        value = value * 16 + digit;
        ++offset;
        ++digits;
      }
      if (digits) {
        if (offset < text.size() && std::string_view(" \t\r\n\f").find(text[offset]) != std::string_view::npos)
          ++offset;
      } else {
        value = static_cast<unsigned char>(text[offset++]);
        if (value == '\r' || value == '\n' || value == '\f') return invalid();
      }
    }
    if (value >= 'A' && value <= 'Z') value += 'a' - 'A';
    if (value != static_cast<unsigned char>(expected)) return invalid();
  }
  return offset == text.size() ? Result::ok(true) : invalid();
};

}  // namespace scss
