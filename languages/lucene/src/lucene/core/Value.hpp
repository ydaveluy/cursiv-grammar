#pragma once
#include "lucene/core/ast.hpp"
#include <charconv>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
namespace lucene {
struct TextValue {
  std::string text;
  std::string error;
};
inline TextValue term_value(std::string_view raw) {
  TextValue result;
  auto hex = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return -1;
  };
  auto unit = [&](std::size_t pos) -> std::optional<unsigned> {
    if (pos + 4 > raw.size())
      return {};
    unsigned value = 0;
    for (std::size_t k = 0; k < 4; ++k) {
      const int digit = hex(raw[pos + k]);
      if (digit < 0)
        return {};
      value = value * 16 + digit;
    }
    return value;
  };
  auto emit = [&](unsigned cp) {
    if (cp < 0x80)
      result.text += char(cp);
    else if (cp < 0x800) {
      result.text += char(0xc0 | (cp >> 6));
      result.text += char(0x80 | (cp & 63));
    } else if (cp < 0x10000) {
      result.text += char(0xe0 | (cp >> 12));
      result.text += char(0x80 | ((cp >> 6) & 63));
      result.text += char(0x80 | (cp & 63));
    } else {
      result.text += char(0xf0 | (cp >> 18));
      result.text += char(0x80 | ((cp >> 12) & 63));
      result.text += char(0x80 | ((cp >> 6) & 63));
      result.text += char(0x80 | (cp & 63));
    }
  };
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] != '\\') {
      result.text += raw[i];
      continue;
    }
    if (++i == raw.size()) {
      result.error = "A term cannot end with an escape character.";
      return result;
    }
    if (raw[i] != 'u') {
      result.text += raw[i];
      continue;
    }
    const auto first = unit(i + 1);
    if (!first) {
      result.error = "Expected four hexadecimal digits after '\\u'.";
      return result;
    }
    unsigned cp = *first;
    i += 4;
    if (cp >= 0xd800 && cp <= 0xdbff && i + 6 < raw.size() &&
        raw[i + 1] == '\\' && raw[i + 2] == 'u') {
      const auto second = unit(i + 3);
      if (second && *second >= 0xdc00 && *second <= 0xdfff) {
        cp = 0x10000 + ((cp - 0xd800) << 10) + *second - 0xdc00;
        i += 6;
      }
    }
    if (cp >= 0xd800 && cp <= 0xdfff)
      cp = 0xfffd;
    emit(cp);
  }
  return result;
}
inline TextValue quoted_value(const ast::QuotedTerm &n) {
  return term_value(n.raw.substr(1, n.raw.size() - 2));
}
inline TextValue bound_value(const ast::RangeBound &n) {
  return term_value(n.quoted ? n.raw.substr(1, n.raw.size() - 2) : n.raw);
}
inline std::optional<float> suffix_number(std::string_view raw) {
  if (raw.empty())
    return {};
  if (raw.front() == '+')
    raw.remove_prefix(1);
  if (raw == "NaN" || raw == "-NaN")
    return std::numeric_limits<float>::quiet_NaN();
  if (raw == "Infinity")
    return std::numeric_limits<float>::infinity();
  if (raw == "-Infinity")
    return -std::numeric_limits<float>::infinity();
  if (!raw.empty() && (raw.back() == 'f' || raw.back() == 'F' ||
                       raw.back() == 'd' || raw.back() == 'D'))
    raw.remove_suffix(1);
  bool negative = !raw.empty() && raw.front() == '-';
  auto number = raw;
  if (negative)
    number.remove_prefix(1);
  const bool hex = number.starts_with("0x") || number.starts_with("0X");
  if (hex) {
    if (number.find_first_of("pP") == number.npos)
      return {};
    number.remove_prefix(2);
  }
  float value = 0;
  auto parsed = std::from_chars(
      number.data(), number.data() + number.size(), value,
      hex ? std::chars_format::hex : std::chars_format::general);
  if (parsed.ptr != number.data() + number.size())
    return {};
  if (parsed.ec == std::errc::result_out_of_range) {
    const auto e = number.find_first_of(hex ? "pP" : "eE");
    value = e != number.npos && e + 1 < number.size() && number[e + 1] == '-'
                ? 0
                : std::numeric_limits<float>::infinity();
  } else if (parsed.ec != std::errc())
    return {};
  return negative ? -value : value;
}
inline bool has_wildcard(std::string_view raw) {
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\\' && i + 1 < raw.size())
      ++i;
    else if (raw[i] == '*' || raw[i] == '?')
      return true;
  }
  return false;
}
} // namespace lucene
