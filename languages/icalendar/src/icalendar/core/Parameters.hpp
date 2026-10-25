#pragma once

#include <memory_resource>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace icalendar {
inline constexpr auto trim_ows = [](std::string_view text) {
  const auto begin = text.find_first_not_of(" \t");
  if (begin == text.npos) return std::string_view{};
  return text.substr(begin, text.find_last_not_of(" \t") - begin + 1);
};
inline std::string_view trim_name_edges(std::string_view text) {
  static constexpr std::string_view spaces[] = {
      "\u0085", "\u00a0", "\u1680", "\u2000", "\u2001", "\u2002", "\u2003",
      "\u2004", "\u2005", "\u2006", "\u2007", "\u2008", "\u2009", "\u200a",
      "\u2028", "\u2029", "\u202f", "\u205f", "\u3000"};
  const auto ascii_space = [](unsigned char c) {
    return (c >= 9 && c <= 13) || (c >= 0x1C && c <= 0x20);
  };
  bool changed = true;
  while (changed && !text.empty()) {
    changed = false;
    if (ascii_space(text.front())) { text.remove_prefix(1); changed = true; }
    else for (auto space : spaces)
      if (text.starts_with(space)) { text.remove_prefix(space.size()); changed = true; break; }
  }
  changed = true;
  while (changed && !text.empty()) {
    changed = false;
    if (ascii_space(text.back())) { text.remove_suffix(1); changed = true; }
    else for (auto space : spaces)
      if (text.ends_with(space)) { text.remove_suffix(space.size()); changed = true; break; }
  }
  return text;
}
inline constexpr auto property_name =
    [](std::string_view text, std::pmr::memory_resource* mem) -> std::string_view {
  text = trim_name_edges(text);
  if (text.find_first_of(" \t") == text.npos) return text;
  const auto room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  for (char c : text)
    if (c != ' ' && c != '\t') room[n++] = c;
  return {room.data(), n};
};

inline void replace_all(std::string& text, std::string_view from,
                        std::string_view to) {
  for (std::size_t at = 0; (at = text.find(from, at)) != text.npos; at += to.size())
    text.replace(at, from.size(), to);
}

inline std::vector<std::string> parameter_values(std::string_view raw) {
  std::string text(raw);
  replace_all(text, "%", "%25");
  replace_all(text, "\\,", "%2C");
  replace_all(text, "\\:", "%3A");
  replace_all(text, "\\;", "%3B");
  replace_all(text, "\\\\", "%5C");
  std::vector<std::string> result;
  bool quoted = false;
  std::size_t begin = 0;
  for (std::size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] == '"') quoted = !quoted;
    if (i < text.size() && (text[i] != ',' || quoted)) continue;
    auto part = std::string_view(text).substr(begin, i - begin);
    if (part.size() >= 2 && part.front() == '"' && part.back() == '"') {
      while (!part.empty() && part.front() == '"') part.remove_prefix(1);
      while (!part.empty() && part.back() == '"') part.remove_suffix(1);
    }
    std::string value;
    for (std::size_t j = 0; j < part.size(); ++j) {
      if (part[j] == '^' && j + 1 < part.size()) {
        const char next = part[j + 1];
        if (next == '^' || next == 'n' || next == '\'') {
          value += next == 'n' ? '\n' : next == '\'' ? '"' : '^';
          ++j;
          continue;
        }
      }
      value += part[j];
    }
    replace_all(value, "%2C", ",");
    replace_all(value, "%3A", ":");
    replace_all(value, "%3B", ";");
    replace_all(value, "%5C", "\\");
    replace_all(value, "%25", "%");
    result.push_back(std::move(value));
    begin = i + 1;
  }
  return result;
}
}  // namespace icalendar
