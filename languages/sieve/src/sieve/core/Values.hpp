#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include "sieve/core/ast.hpp"

namespace sieve {
inline std::optional<std::uint64_t> number_value(const ast::Number &number) {
  auto text = number.text;
  if (text.empty())
    return std::nullopt;
  unsigned shift = 0;
  if (!text.empty()) {
    const char suffix = text.back() | 0x20;
    if (suffix == 'k' || suffix == 'm' || suffix == 'g') {
      shift = suffix == 'k' ? 10 : suffix == 'm' ? 20 : 30;
      text.remove_suffix(1);
    }
  }
  std::uint64_t value = 0;
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
      value > (std::numeric_limits<std::uint64_t>::max() >> shift))
    return std::nullopt;
  return value << shift;
}
inline std::string string_value(const ast::String &string) {
  auto raw = string.raw;
  std::string result;
  if (raw.empty())
    return result;
  const bool quoted = raw.front() == '"';
  if (quoted) {
    if (raw.size() < 2)
      return result;
    raw.remove_prefix(1);
    raw.remove_suffix(1);
  } else {
    const auto first = raw.find('\n');
    if (first == std::string_view::npos)
      return result;
    raw.remove_prefix(first + 1);
    if (raw.ends_with("\r\n"))
      raw.remove_suffix(2);
    else if (raw.ends_with("\n"))
      raw.remove_suffix(1);
    if (!raw.empty())
      raw.remove_suffix(1);
  }
  bool line_start = true;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    char c = raw[i];
    if (quoted && c == '\\' && i + 1 < raw.size())
      c = raw[++i];
    if (!quoted && line_start && c == '.' && i + 1 < raw.size() &&
        raw[i + 1] == '.')
      ++i;
    if (c == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n')
      continue;
    if (c == '\n')
      result += '\r';
    result += c;
    line_start = c == '\n';
  }
  return result;
}
} // namespace sieve
