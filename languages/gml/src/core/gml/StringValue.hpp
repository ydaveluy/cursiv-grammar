#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

#include "cursiv/grammar/Conversion.hpp"
#include "gml/NamedEntities.hpp"

namespace gml {

inline constexpr auto decode_string = [](std::string_view literal) -> std::string {
  if (literal.size() < 2) return {};
  auto text = literal.substr(1, literal.size() - 2);
  const auto whitespace = [](char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
           c == '\v' || c == '\f';
  };
  std::string joined;
  if (text.find_first_of("\r\n") != std::string_view::npos) {
    joined.reserve(text.size());
    bool first = true;
    while (true) {
      const auto end = text.find_first_of("\r\n");
      auto line = text.substr(0, end);
      if (!first)
        while (!line.empty() && whitespace(line.front())) line.remove_prefix(1);
      if (end != std::string_view::npos)
        while (!line.empty() && whitespace(line.back())) line.remove_suffix(1);
      joined += line;
      if (end == std::string_view::npos) break;
      joined += ' ';
      const auto separator = text[end] == '\r' && end + 1 < text.size() &&
                                     text[end + 1] == '\n' ? 2u : 1u;
      text.remove_prefix(end + separator);
      first = false;
    }
    text = joined;
  }
  if (text.find('&') == std::string_view::npos) return std::string(text);
  std::string out;
  out.reserve(text.size());
  for (std::size_t i = 0; i < text.size();) {
    const auto end = text[i] == '&' ? text.find(';', i + 1) : std::string_view::npos;
    std::uint32_t code = 0;
    bool found = false;
    if (end != std::string_view::npos) {
      auto name = text.substr(i + 1, end - i - 1);
      if (name.starts_with('#')) {
        name.remove_prefix(1);
        int base = 10;
        if (name.starts_with('x')) { name.remove_prefix(1); base = 16; }
        if (!name.empty()) {
          const auto result = std::from_chars(name.data(), name.data() + name.size(), code, base);
          found = result.ec == std::errc{} && result.ptr == name.data() + name.size();
        }
      } else {
        const auto entity = std::lower_bound(
            named_entities.begin(), named_entities.end(), name,
            [](const auto& entry, std::string_view key) { return entry.first < key; });
        if (entity != named_entities.end() && entity->first == name) {
          code = entity->second;
          found = true;
        }
      }
      found &= code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff);
    }
    if (!found) { out += text[i++]; continue; }
    if (code <= 0x7f) out += static_cast<char>(code);
    else {
      if (code <= 0x7ff) out += static_cast<char>(0xc0 | (code >> 6));
      else {
        if (code <= 0xffff) out += static_cast<char>(0xe0 | (code >> 12));
        else {
          out += static_cast<char>(0xf0 | (code >> 18));
          out += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
        }
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
      }
      out += static_cast<char>(0x80 | (code & 0x3f));
    }
    i = end + 1;
  }
  return out;
};

}  // namespace gml
