#pragma once
#include <cstdint>
#include <string_view>
#include <memory_resource>
#include "cursiv/core/grammar/Conversion.hpp"
#include "cursiv/core/grammar/unicode/UnicodeRanges.hpp"
#include "cursiv/core/text/scalar_at.hpp"
namespace javascript {
inline bool identifier_character(char32_t cp, bool first) {
  using P = cursiv::unicode::Property;
  const auto property = [&](P p) {
    const auto ranges = cursiv::unicode::kTables[static_cast<unsigned>(p)];
    return cursiv::unicode::within(ranges.data(), ranges.size(), cp);
  };
  const bool start = cp == '_' || cp == '$' || property(P::L) || property(P::Nl) ||
      cp == 0x1885 || cp == 0x1886 || cp == 0x2118 || cp == 0x212e || cp == 0x309b || cp == 0x309c;
  if (first || start) return start;
  return property(P::Mn) || property(P::Mc) || property(P::Nd) || property(P::Pc) ||
      cp == 0x200c || cp == 0x200d || cp == 0xb7 || cp == 0x387 ||
      (cp >= 0x1369 && cp <= 0x1371) || cp == 0x19da || cp == 0x30fb || cp == 0xff65;
}
inline constexpr auto decode_identifier = [](std::string_view text, std::pmr::memory_resource* memory)
    -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.find('\\') == std::string_view::npos) return Result::ok(text);
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t at = 0, size = 0;
  if (text.starts_with('#')) { room[size++] = '#'; ++at; }
  bool first = true;
  while (at < text.size()) {
    char32_t cp = 0;
    if (text[at] == '\\') {
      if (at + 2 >= text.size() || text[at + 1] != 'u')
        return Result::err("Incomplete identifier escape");
      at += 2; // The token grammar has consumed the mandatory backslash-u.
      const bool braced = text[at] == '{';
      if (braced) ++at;
      unsigned count = 0;
      while (at < text.size() && (braced ? text[at] != '}' : count < 4)) {
        const char h = text[at++];
        if (!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F')))
          return Result::err("Invalid hexadecimal identifier escape");
        const unsigned digit = h <= '9' ? unsigned(h - '0') : unsigned((h | 32) - 'a' + 10);
        if (cp > (0x10ffffu - digit) / 16)
          return Result::err("An identifier escape exceeds U+10FFFF");
        cp = cp * 16 + digit; ++count;
      }
      if (!count || (braced ? at == text.size() : count != 4))
        return Result::err("Incomplete identifier escape");
      if (braced) ++at;
    } else {
      std::size_t width = 0;
      cp = cursiv::scalar_at(text.data() + at, text.size() - at, width);
      at += width;
    }
    if (!identifier_character(cp, first))
      return Result::err("An escaped character is not valid at this identifier position");
    first = false;
    if (cp < 0x80) room[size++] = char(cp);
    else if (cp < 0x800) {
      room[size++] = char(0xc0 | (cp >> 6)); room[size++] = char(0x80 | (cp & 63));
    } else if (cp < 0x10000) {
      room[size++] = char(0xe0 | (cp >> 12)); room[size++] = char(0x80 | ((cp >> 6) & 63)); room[size++] = char(0x80 | (cp & 63));
    } else {
      room[size++] = char(0xf0 | (cp >> 18)); room[size++] = char(0x80 | ((cp >> 12) & 63));
      room[size++] = char(0x80 | ((cp >> 6) & 63)); room[size++] = char(0x80 | (cp & 63));
    }
  }
  return Result::ok(std::string_view(room.data(), size));
};
} // namespace javascript
