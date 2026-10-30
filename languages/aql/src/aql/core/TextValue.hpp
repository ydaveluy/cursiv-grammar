#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace aql {

// AQL uses the same backslash escapes in both strings and quoted names.
// Unrecognized escapes drop the backslash, as in ArangoDB's UTF-8 decoder.
inline constexpr auto text_value =
    [](std::string_view text, std::pmr::memory_resource* mem) {
      if (text.empty() || (text.front() != '\'' && text.front() != '"' &&
                           text.front() != '`' &&
                           !text.starts_with("´")))
        return text;
      const std::size_t quote = text.starts_with("´") ? 2 : 1;
      const std::string_view body = text.substr(quote, text.size() - 2 * quote);
      if (body.find('\\') == std::string_view::npos) return body;
      const auto room = cursiv::text_room(mem, body.size());
      std::size_t used = 0;
      const auto hex = [](char c) -> std::uint32_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return 0;
      };
      const auto unit = [&](std::size_t pos) {
        std::uint32_t value = 0;
        for (std::size_t n = 0; n != 4; ++n)
          value = (value << 4) | hex(body[pos + n]);
        return value;
      };
      const auto emit = [&](std::uint32_t value) {
        if (value < 0x80) {
          room[used++] = static_cast<char>(value);
        } else if (value < 0x800) {
          room[used++] = static_cast<char>(0xc0 | (value >> 6));
          room[used++] = static_cast<char>(0x80 | (value & 0x3f));
        } else if (value < 0x10000) {
          room[used++] = static_cast<char>(0xe0 | (value >> 12));
          room[used++] = static_cast<char>(0x80 | ((value >> 6) & 0x3f));
          room[used++] = static_cast<char>(0x80 | (value & 0x3f));
        } else {
          room[used++] = static_cast<char>(0xf0 | (value >> 18));
          room[used++] = static_cast<char>(0x80 | ((value >> 12) & 0x3f));
          room[used++] = static_cast<char>(0x80 | ((value >> 6) & 0x3f));
          room[used++] = static_cast<char>(0x80 | (value & 0x3f));
        }
      };
      for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i] != '\\' || i + 1 == body.size()) {
          room[used++] = body[i];
          continue;
        }
        const char escaped = body[++i];
        switch (escaped) {
          case 'b': room[used++] = '\b'; break;
          case 'f': room[used++] = '\f'; break;
          case 'n': room[used++] = '\n'; break;
          case 'r': room[used++] = '\r'; break;
          case 't': room[used++] = '\t'; break;
          case 'u': {
            if (i + 4 >= body.size()) {
              room[used++] = escaped;
              break;
            }
            std::uint32_t value = unit(i + 1);
            i += 4;
            if (value >= 0xd800 && value <= 0xdbff &&
                i + 6 < body.size() && body[i + 1] == '\\' &&
                body[i + 2] == 'u') {
              const auto low = unit(i + 3);
              if (low >= 0xdc00 && low <= 0xdfff) {
                value = 0x10000 + ((value - 0xd800) << 10) + low - 0xdc00;
                i += 6;
              }
            }
            emit(value);
            break;
          }
          default: room[used++] = escaped; break;
        }
      }
      return std::string_view(room.data(), used);
    };

inline constexpr auto radix_number = [](std::string_view text) {
  const unsigned base = text[1] == 'b' || text[1] == 'B' ? 2 : 16;
  if (text.size() - 2 > (base == 2 ? 32 : 8))
    return cursiv::ConversionResult<double>::err(
        "AQL binary and hexadecimal literals are limited to 32 bits");
  double value = 0;
  for (const char c : text.substr(2)) {
    const unsigned digit = c <= '9' ? c - '0' :
                           c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
    value = value * base + digit;
  }
  return cursiv::ConversionResult<double>::ok(value);
};

}  // namespace aql
