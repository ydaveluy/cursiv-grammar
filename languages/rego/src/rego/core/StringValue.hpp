#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace rego {
inline unsigned hex_nibble(char c) {
  if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
  if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
  return static_cast<unsigned>(c - 'A' + 10);
}
inline unsigned hex_quad(std::string_view s, std::size_t i) {
  return (hex_nibble(s[i]) << 12) | (hex_nibble(s[i + 1]) << 8) |
         (hex_nibble(s[i + 2]) << 4) | hex_nibble(s[i + 3]);
}
inline void append_utf8(std::span<char> room, std::size_t& n, unsigned cp) {
  if (cp < 0x80)
    room[n++] = static_cast<char>(cp);
  else if (cp < 0x800) {
    room[n++] = static_cast<char>(0xC0 | (cp >> 6));
    room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    room[n++] = static_cast<char>(0xE0 | (cp >> 12));
    room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    room[n++] = static_cast<char>(0xF0 | (cp >> 18));
    room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    room[n++] = static_cast<char>(0x80 | (cp & 0x3F));
  }
}
inline constexpr auto unescape_string =
    [](std::string_view t, std::pmr::memory_resource* mem) -> std::string_view {
  const auto body = t.substr(1, t.size() - 2);
  if (t.front() == '`' || body.find('\\') == std::string_view::npos)
    return body;
  const auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') {
      room[n++] = body[i];
      continue;
    }
    switch (body[++i]) {
      case 'b':
        room[n++] = '\b';
        break;
      case 'f':
        room[n++] = '\f';
        break;
      case 'n':
        room[n++] = '\n';
        break;
      case 'r':
        room[n++] = '\r';
        break;
      case 't':
        room[n++] = '\t';
        break;
      case 'u': {
        unsigned cp = hex_quad(body, i + 1);
        i += 4;
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 < body.size() &&
            body[i + 1] == '\\' && body[i + 2] == 'u') {
          const unsigned low = hex_quad(body, i + 3);
          if (low >= 0xDC00 && low <= 0xDFFF) {
            cp = 0x10000 + ((cp - 0xD800) << 10) + low - 0xDC00;
            i += 6;
          }
        }
        if (cp >= 0xD800 && cp <= 0xDFFF) cp = 0xFFFD;
        append_utf8(room, n, cp);
        break;
      }
      default:
        room[n++] = body[i];
        break;
    }
  }
  return {room.data(), n};
};
}  // namespace rego
