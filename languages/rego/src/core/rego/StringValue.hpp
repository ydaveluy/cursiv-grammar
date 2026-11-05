#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "cursiv/grammar/GrammarSpec.hpp"

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
inline void append_utf8(std::string& out, unsigned cp) {
  if (cp < 0x80)
    out += static_cast<char>(cp);
  else if (cp < 0x800) {
    out += static_cast<char>(0xC0 | (cp >> 6));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    out += static_cast<char>(0xF0 | (cp >> 18));
    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }
}
inline constexpr auto unescape_string =
    [](std::string_view t) -> std::string {
  const auto body = t.substr(1, t.size() - 2);
  if (t.front() == '`' || body.find('\\') == std::string_view::npos)
    return std::string(body);
  std::string out;
  out.reserve(body.size());
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') {
      out += body[i];
      continue;
    }
    switch (body[++i]) {
      case 'b':
        out += '\b';
        break;
      case 'f':
        out += '\f';
        break;
      case 'n':
        out += '\n';
        break;
      case 'r':
        out += '\r';
        break;
      case 't':
        out += '\t';
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
        append_utf8(out, cp);
        break;
      }
      default:
        out += body[i];
        break;
    }
  }
  return out;
};
}  // namespace rego
