#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/Conversion.hpp"
#include "cursiv/core/grammar/unicode/UnicodeRanges.hpp"
#include "cursiv/core/text/scalar_at.hpp"

namespace typescript {

inline bool identifier_character(char32_t cp, bool first) {
  using P = cursiv::unicode::Property;
  const auto property = [&](P p) {
    const auto ranges = cursiv::unicode::kTables[static_cast<unsigned>(p)];
    return cursiv::unicode::within(ranges.data(), ranges.size(), cp);
  };
  const bool start = cp == '_' || cp == '$' || property(P::L) || property(P::Nl) ||
      cp == 0x1885 || cp == 0x1886 || cp == 0x2118 || cp == 0x212e ||
      cp == 0x309b || cp == 0x309c;
  if (first || start) return start;
  return property(P::Mn) || property(P::Mc) || property(P::Nd) || property(P::Pc) ||
      cp == 0x200c || cp == 0x200d || cp == 0xb7 || cp == 0x387 ||
      (cp >= 0x1369 && cp <= 0x1371) || cp == 0x19da ||
      cp == 0x30fb || cp == 0xff65;
}

// The lexer checks literal characters. Escapes must denote the same character
// classes, while the AST continues to store their original source spelling.
inline constexpr auto validate_identifier = [](std::string_view text)
    -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.find('\\') == text.npos) return Result::ok(text);
  std::size_t at = text.starts_with('#') ? 1 : 0;
  bool first = true;
  while (at < text.size()) {
    char32_t cp;
    if (text[at] == '\\') {
      if (!text.substr(at).starts_with("\\u"))
        return Result::err("Expected a Unicode identifier escape.");
      at += 2;
      const bool braced = at < text.size() && text[at] == '{';
      if (braced) ++at;
      const auto end = braced ? text.find('}', at) : at + 4;
      if (end == text.npos || end > text.size() || end == at)
        return Result::err("Incomplete identifier escape.");
      std::uint32_t value = 0;
      const auto result = std::from_chars(text.data() + at, text.data() + end, value, 16);
      if (result.ec != std::errc{} || result.ptr != text.data() + end ||
          value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        return Result::err("An identifier escape must denote a Unicode scalar.");
      cp = value;
      at = end + (braced ? 1 : 0);
    } else {
      std::size_t width = 0;
      cp = cursiv::scalar_at(text.data() + at, text.size() - at, width);
      if (!width) return Result::err("Invalid identifier character.");
      at += width;
    }
    if (!identifier_character(cp, first))
      return Result::err("The escaped character is invalid at this identifier position.");
    first = false;
  }
  return Result::ok(text);
};

}  // namespace typescript
