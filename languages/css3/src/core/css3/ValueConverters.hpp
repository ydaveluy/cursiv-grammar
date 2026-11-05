#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include "cursiv/grammar/Conversion.hpp"

namespace css3 {

inline std::string decode_css(std::string_view body) {
  if (body.find_first_of("\\\r\f\0", 0, 4) == std::string_view::npos)
    return std::string(body);
  std::string out;
  out.reserve(body.size());
  const auto hex = [](char c) -> unsigned {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 16;
  };
  const auto scalar = [&](std::uint32_t c) {
    if (c == 0 || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) c = 0xfffd;
    if (c < 0x80) out += static_cast<char>(c);
    else {
      const unsigned bytes = c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
      out += static_cast<char>((0xffu << (8 - bytes)) | (c >> (6 * (bytes - 1))));
      for (unsigned j = bytes - 1; j > 0; --j)
        out += static_cast<char>(0x80 | ((c >> (6 * (j - 1))) & 63));
    }
  };
  for (std::size_t i = 0; i < body.size(); ++i) {
    char c = body[i];
    if (c == '\\' && i + 1 < body.size()) {
      c = body[++i];
      if (c == '\r' || c == '\n' || c == '\f') {
        if (c == '\r' && i + 1 < body.size() && body[i + 1] == '\n') ++i;
        continue;
      }
      if (hex(c) < 16) {
        std::uint32_t value = hex(c);
        for (unsigned j = 1; j < 6 && i + 1 < body.size() && hex(body[i + 1]) < 16; ++j)
          value = value * 16 + hex(body[++i]);
        if (i + 1 < body.size() && body.substr(i + 1, 1).find_first_of(" \t\r\n\f") != std::string_view::npos) {
          c = body[++i];
          if (c == '\r' && i + 1 < body.size() && body[i + 1] == '\n') ++i;
        }
        scalar(value);
        continue;
      }
    }
    if (c == 0) scalar(0);
    else if (c == '\r' || c == '\f') {
      if (c == '\r' && i + 1 < body.size() && body[i + 1] == '\n') ++i;
      out += '\n';
    } else out += c;
  }
  return out;
}

inline constexpr auto unquote = [](std::string_view text) -> cursiv::ConversionResult<std::string> {
  using Result = cursiv::ConversionResult<std::string>;
  if (text.size() < 2 || (text.front() != '\'' && text.front() != '"') ||
      text.back() != text.front())
    return Result::err("A CSS string needs matching quotes.");
  return Result::ok(decode_css(text.substr(1, text.size() - 2)));
};

}  // namespace css3
