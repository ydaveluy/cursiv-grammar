// Decode RON string, character and byte-string literals into the parse arena.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/Conversion.hpp"

namespace ron {
namespace detail {
inline bool valid_utf8(std::string_view text) {
  for (std::size_t i = 0; i < text.size();) {
    const auto lead = static_cast<unsigned char>(text[i++]);
    if (lead < 0x80) continue;
    unsigned trailing = 0;
    std::uint32_t scalar = 0, minimum = 0;
    if (lead >= 0xc2 && lead <= 0xdf) {
      trailing = 1; scalar = lead & 0x1f; minimum = 0x80;
    } else if (lead >= 0xe0 && lead <= 0xef) {
      trailing = 2; scalar = lead & 0xf; minimum = 0x800;
    } else if (lead >= 0xf0 && lead <= 0xf4) {
      trailing = 3; scalar = lead & 7; minimum = 0x10000;
    } else return false;
    if (text.size() - i < trailing) return false;
    while (trailing--) {
      const auto byte = static_cast<unsigned char>(text[i++]);
      if ((byte & 0xc0) != 0x80) return false;
      scalar = (scalar << 6) | (byte & 0x3f);
    }
    if (scalar < minimum || scalar > 0x10ffff ||
        (scalar >= 0xd800 && scalar <= 0xdfff)) return false;
  }
  return true;
}
}  // namespace detail

inline constexpr auto unquote =
    [](std::string_view t, std::pmr::memory_resource *mem)
        -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (!detail::valid_utf8(t))
    return Result::err("RON literals must contain valid UTF-8 source text.");
  const bool binary = t.starts_with('b');
  const auto result = [binary](std::string_view value) {
    if (!binary && !detail::valid_utf8(value))
      return Result::err("String escapes must form valid UTF-8 text.");
    return Result::ok(value);
  };
  if (t.starts_with("r") || t.starts_with("br")) {
    const auto first = t.find('"'), last = t.rfind('"');
    return result(t.substr(first + 1, last - first - 1));
  }
  if (t.starts_with("b"))
    t.remove_prefix(1);
  t = t.substr(1, t.size() - 2);
  if (t.find('\\') == std::string_view::npos)
    return result(t);
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  bool valid = true;
  const auto hex = [&](std::string_view digits) {
    std::uint32_t cp = 0;
    const auto [end, ec] =
        std::from_chars(digits.data(), digits.data() + digits.size(), cp, 16);
    valid &= ec == std::errc{} && end == digits.data() + digits.size();
    return cp;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\' || i + 1 == t.size()) {
      room[n++] = t[i];
      continue;
    }
    const char escaped = t[++i];
    if (escaped == '\n' || escaped == '\r') {
      if (escaped == '\r' && i + 1 < t.size() && t[i + 1] == '\n') ++i;
      while (i + 1 < t.size() &&
             std::string_view(" \t\r\n").find(t[i + 1]) != std::string_view::npos)
        ++i;
      continue;
    }
    if (escaped == 'x') {
      room[n++] = static_cast<char>(hex(t.substr(i + 1, 2)));
      i += 2;
    } else if (escaped == 'u') {
      const auto end = t.find('}', i + 2);
      const auto digits = t.substr(i + 2, end - i - 2);
      if (digits.size() > 6)
        return Result::err("Unicode escape must contain one to six hexadecimal digits.");
      const auto cp = hex(digits);
      i = end;
      if (!valid || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
        return Result::err("Unicode escape must denote a Unicode scalar value.");
      if (cp < 0x80)
        room[n++] = static_cast<char>(cp);
      else {
        if (cp >= 0x10000)
          room[n++] = static_cast<char>(0xf0 | (cp >> 18));
        else if (cp >= 0x800)
          room[n++] = static_cast<char>(0xe0 | (cp >> 12));
        else
          room[n++] = static_cast<char>(0xc0 | (cp >> 6));
        if (cp >= 0x10000)
          room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
        if (cp >= 0x800)
          room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
        room[n++] = static_cast<char>(0x80 | (cp & 63));
      }
    } else {
      switch (escaped) {
      case '0':
        room[n++] = '\0';
        break;
      case 'b':
      case 'f':
        return Result::err("Unknown RON string escape.");
      case 'n':
        room[n++] = '\n';
        break;
      case 'r':
        room[n++] = '\r';
        break;
      case 't':
        room[n++] = '\t';
        break;
      default:
        room[n++] = escaped;
        break;
      }
    }
  }
  return result({room.data(), n});
};

}  // namespace ron
