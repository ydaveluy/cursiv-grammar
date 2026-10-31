#pragma once

#include <string_view>
#include "cursiv/core/grammar/Conversion.hpp"

namespace scss {

inline constexpr auto unquote = [](std::string_view t) -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (t.size() < 2 || (t.front() != '\'' && t.front() != '"') || t.back() != t.front())
    return Result::err("A string needs matching quotes.");
  return Result::ok(t.substr(1, t.size() - 2));
};

inline constexpr auto trim_url = [](std::string_view text) {
  const auto blank = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f'; };
  while (!text.empty() && blank(text.front())) text.remove_prefix(1);
  while (!text.empty() && blank(text.back())) text.remove_suffix(1);
  return text;
};
inline constexpr auto quoted_url = [](std::string_view text) -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto trivia = [&](std::size_t& at) {
    while (at < text.size()) {
      if (text.substr(at).starts_with("/*")) {
        const auto end = text.find("*/", at + 2);
        if (end == text.npos) return false;
        at = end + 2;
      } else if (text[at] == ' ' || text[at] == '\t' || text[at] == '\r' ||
                 text[at] == '\n' || text[at] == '\f') ++at;
      else break;
    }
    return true;
  };
  std::size_t start = 0;
  if (!trivia(start) || start == text.size() ||
      (text[start] != '\'' && text[start] != '"'))
    return Result::err("A quoted URL needs a quoted string.");
  const char quote = text[start];
  std::size_t end = start + 1;
  while (end < text.size()) {
    if (text[end] == '\\') {
      if (++end == text.size()) return Result::err("Incomplete URL string escape.");
      ++end;
    } else if (text[end++] == quote) {
      auto after = end;
      if (!trivia(after) || after != text.size())
        return Result::err("Unexpected text after the quoted URL.");
      return unquote(text.substr(start, end - start));
    }
  }
  return Result::err("A quoted URL needs its closing quote.");
};

inline constexpr auto drop_parent = [](std::string_view t) -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (t.size() < 2 || t.front() != '&')
    return Result::err("A parent suffix needs its ampersand and suffix.");
  return Result::ok(t.substr(1));
};

}  // namespace scss
