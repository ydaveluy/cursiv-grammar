#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace properties {
struct DecodedText {
  std::string text;
  std::size_t invalid_unicode = std::string_view::npos;
};

// Properties first joins continued physical lines, then decodes escapes.
// Source offsets keep malformed Unicode diagnostics on the original spelling.
inline DecodedText decode_text(std::string_view raw) {
  std::string line;
  std::vector<std::size_t> offsets;
  auto put_raw = [&](std::size_t i) { line += raw[i]; offsets.push_back(i); };
  std::size_t i = 0;
  while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t' || raw[i] == '\f')) ++i;
  while (i < raw.size()) {
    if (raw[i] != '\\') { put_raw(i++); continue; }
    if (i + 1 == raw.size()) break; // LineReader drops an odd final backslash.
    if (raw[i + 1] == '\r' || raw[i + 1] == '\n') {
      ++i;
      const char nl = raw[i++];
      if (nl == '\r' && i < raw.size() && raw[i] == '\n') ++i;
      while (i < raw.size() && (raw[i] == ' ' || raw[i] == '\t' || raw[i] == '\f')) ++i;
      continue;
    }
    put_raw(i++); put_raw(i++);
  }
  DecodedText result;
  auto scalar = [&](std::uint32_t cp) {
    if (cp < 0x80) result.text += static_cast<char>(cp);
    else if (cp < 0x800) {
      result.text += static_cast<char>(0xc0 | cp >> 6);
      result.text += static_cast<char>(0x80 | (cp & 63));
    } else if (cp <= 0xffff) {
      result.text += static_cast<char>(0xe0 | cp >> 12);
      result.text += static_cast<char>(0x80 | (cp >> 6 & 63));
      result.text += static_cast<char>(0x80 | (cp & 63));
    } else {
      result.text += static_cast<char>(0xf0 | cp >> 18);
      result.text += static_cast<char>(0x80 | (cp >> 12 & 63));
      result.text += static_cast<char>(0x80 | (cp >> 6 & 63));
      result.text += static_cast<char>(0x80 | (cp & 63));
    }
  };
  auto hex = [&](std::size_t at, std::uint32_t& cp) {
    if (at + 4 > line.size()) return false;
    cp = 0;
    for (std::size_t k = 0; k < 4; ++k) {
      const char c = line[at + k];
      const int d = c >= '0' && c <= '9' ? c - '0' :
                    c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                    c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
      if (d < 0) return false;
      cp = cp * 16 + static_cast<unsigned>(d);
    }
    return true;
  };
  for (i = 0; i < line.size(); ++i) {
    if (line[i] != '\\') { result.text += line[i]; continue; }
    const auto begin = i;
    const char c = line[++i]; // Every retained backslash has a following byte.
    if (c == 'u') {
      std::uint32_t cp;
      if (!hex(i + 1, cp)) {
        if (result.invalid_unicode == std::string_view::npos)
          result.invalid_unicode = offsets[begin];
        result.text += "\\u";
        continue;
      }
      i += 4;
      if (cp >= 0xd800 && cp <= 0xdbff && i + 6 < line.size() &&
          line[i + 1] == '\\' && line[i + 2] == 'u') {
        std::uint32_t low;
        if (hex(i + 3, low) && low >= 0xdc00 && low <= 0xdfff) {
          cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
          i += 6;
        }
      }
      scalar(cp);
    } else if (c == 'n') result.text += '\n';
    else if (c == 'r') result.text += '\r';
    else if (c == 't') result.text += '\t';
    else if (c == 'f') result.text += '\f';
    else result.text += c;
  }
  return result;
}
} // namespace properties
