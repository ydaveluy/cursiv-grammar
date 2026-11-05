#pragma once
#include <cstdint>
#include <string>
#include <string_view>
namespace json {
inline std::string decode_string(std::string_view raw) {
  std::string out;
  const auto hex = [](char c) -> unsigned {
    return c <= '9' ? c - '0' : (c | 32) - 'a' + 10;
  };
  const auto unit = [&](std::size_t i) {
    std::uint32_t n = 0;
    for (int j = 0; j < 4; ++j)
      n = n * 16 + hex(raw[i + j]);
    return n;
  };
  const auto utf8 = [&](std::uint32_t c) {
    if (c < 0x80)
      out += static_cast<char>(c);
    else if (c < 0x800) {
      out += static_cast<char>(0xc0 | (c >> 6));
      out += static_cast<char>(0x80 | (c & 63));
    } else if (c < 0x10000) {
      out += static_cast<char>(0xe0 | (c >> 12));
      out += static_cast<char>(0x80 | ((c >> 6) & 63));
      out += static_cast<char>(0x80 | (c & 63));
    } else {
      out += static_cast<char>(0xf0 | (c >> 18));
      out += static_cast<char>(0x80 | ((c >> 12) & 63));
      out += static_cast<char>(0x80 | ((c >> 6) & 63));
      out += static_cast<char>(0x80 | (c & 63));
    }
  };
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] != '\\' || i + 1 == raw.size()) {
      out += raw[i];
      continue;
    }
    const char e = raw[++i];
    if (e == 'u' && i + 4 < raw.size()) {
      auto c = unit(i + 1);
      i += 4;
      if (c >= 0xd800 && c <= 0xdbff && i + 6 < raw.size() &&
          raw[i + 1] == '\\' && raw[i + 2] == 'u') {
        const auto low = unit(i + 3);
        if (low >= 0xdc00 && low <= 0xdfff) {
          c = 0x10000 + ((c - 0xd800) << 10) + low - 0xdc00;
          i += 6;
        }
      }
      utf8(c);
    } else {
      switch (e) {
      case 'b':
        out += '';
        break;
      case 'f':
        out += '';
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
      default:
        out += e;
        break;
      }
    }
  }
  return out;
}
} // namespace json
