#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace cto {

// Composer's string defaults and decorator strings carry their decoded value.
inline constexpr auto string_value = [](std::string_view token,
                                       std::pmr::memory_resource* mem) {
  const auto text = token.substr(1, token.size() - 2);
  if (text.find('\\') == std::string_view::npos) return text;
  auto room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  const auto scalar = [&](std::uint32_t cp) {
    if (cp < 0x80) room[n++] = static_cast<char>(cp);
    else {
      const unsigned bytes = cp < 0x800 ? 2 : cp < 0x10000 ? 3 : 4;
      room[n++] = static_cast<char>((0xffu << (8 - bytes)) | (cp >> (6 * (bytes - 1))));
      for (unsigned j = bytes - 1; j > 0; --j)
        room[n++] = static_cast<char>(0x80 | ((cp >> (6 * (j - 1))) & 63));
    }
  };
  const auto hex = [](std::string_view digits) {
    std::uint32_t cp = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), cp, 16);
    return cp;
  };
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\') { room[n++] = text[i]; continue; }
    const char c = text[++i];
    if (c == '\r' || c == '\n') {
      if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;
      continue;
    }
    if (text.substr(i, 3) == "\xe2\x80\xa8" || text.substr(i, 3) == "\xe2\x80\xa9") {
      i += 2;
      continue;
    }
    if (c == 'x' || c == 'u') {
      while (c == 'u' && i + 1 < text.size() && text[i + 1] == 'u') ++i;
      const unsigned width = c == 'x' ? 2 : 4;
      auto cp = hex(text.substr(i + 1, width));
      i += width;
      if (cp >= 0xd800 && cp <= 0xdbff && text.substr(i + 1, 2) == "\\u" &&
          i + 6 < text.size()) {
        const auto low = hex(text.substr(i + 3, 4));
        if (low >= 0xdc00 && low <= 0xdfff) {
          cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
          i += 6;
        }
      }
      scalar(cp);
    } else if (c >= '0' && c <= '7') {
      unsigned cp = c - '0';
      for (unsigned j = 1; j < (c <= '3' ? 3u : 2u) && i + 1 < text.size() &&
           text[i + 1] >= '0' && text[i + 1] <= '7'; ++j)
        cp = cp * 8 + text[++i] - '0';
      scalar(cp);
    } else {
      switch (c) {
        case 'b': room[n++] = '\b'; break;
        case 't': room[n++] = '\t'; break;
        case 'n': room[n++] = '\n'; break;
        case 'v': room[n++] = '\v'; break;
        case 'f': room[n++] = '\f'; break;
        case 'r': room[n++] = '\r'; break;
        default: room[n++] = c; break;
      }
    }
  }
  return std::string_view(room.data(), n);
};

}  // namespace cto
