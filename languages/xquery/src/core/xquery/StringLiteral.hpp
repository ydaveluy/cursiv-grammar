#pragma once
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include "cursiv/grammar/Conversion.hpp"

namespace xquery {
inline bool xml_character(std::uint32_t c) {
  return c == 9 || c == 10 || c == 13 || (c >= 0x20 && c <= 0xd7ff) ||
      (c >= 0xe000 && c <= 0xfffd) || (c >= 0x10000 && c <= 0x10ffff);
}
inline std::optional<std::uint32_t> character_reference(std::string_view text) {
  if (text.size() < 4 || !text.starts_with("&#") || text.back() != ';') return {};
  const bool hex = text[2] == 'x';
  const auto digits = text.substr(hex ? 3 : 2, text.size() - (hex ? 4 : 3));
  std::uint32_t value = 0;
  const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value, hex ? 16 : 10);
  if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size() || !xml_character(value)) return {};
  return value;
}
inline bool valid_character_references(std::string_view text) {
  for (std::size_t at = 0; (at = text.find("&#", at)) != text.npos;) {
    const auto end = text.find(';', at);
    if (end == text.npos || !character_reference(text.substr(at, end - at + 1))) return false;
    at = end + 1;
  }
  return true;
}
inline constexpr auto unquote_str = [](std::string_view text)
    -> cursiv::ConversionResult<std::string> {
  using Result = cursiv::ConversionResult<std::string>;
  if (text.size() < 2 || (text.front() != '\'' && text.front() != '"') || text.front() != text.back())
    return Result::err("Expected a complete quoted string.");
  const char quote = text.front();
  const auto body = text.substr(1, text.size() - 2);
  if (body.find_first_of("&\r") == body.npos && body.find(quote) == body.npos) return Result::ok(std::string(body));
  std::string out;
  out.reserve(body.size());
  for (std::size_t i = 0; i < body.size(); ++i) {
    const char c = body[i];
    if (c == '&') {
      const auto end = body.find(';', i);
      if (end == body.npos) return Result::err("Unterminated character or entity reference.");
      const auto reference = body.substr(i, end - i + 1);
      std::optional<std::uint32_t> value;
      if (reference == "&lt;") value = '<';
      else if (reference == "&gt;") value = '>';
      else if (reference == "&amp;") value = '&';
      else if (reference == "&quot;") value = '"';
      else if (reference == "&apos;") value = '\'';
      else value = character_reference(reference);
      if (!value) return Result::err("Invalid XML character or entity reference.");
      const auto cp = *value;
      if (cp < 0x80) out += static_cast<char>(cp);
      else if (cp < 0x800) {
        out += static_cast<char>(0xc0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3f));
      } else if (cp < 0x10000) {
        out += static_cast<char>(0xe0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
        out += static_cast<char>(0x80 | (cp & 0x3f));
      } else {
        out += static_cast<char>(0xf0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3f));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
        out += static_cast<char>(0x80 | (cp & 0x3f));
      }
      i = end;
    } else if (c == quote) {
      if (i + 1 == body.size() || body[i + 1] != quote) return Result::err("A quote inside a string must be doubled.");
      out += quote;
      ++i;
    } else if (c == '\r') {
      out += '\n';
      if (i + 1 < body.size() && body[i + 1] == '\n') ++i;
    } else out += c;
  }
  return Result::ok(std::move(out));
};
} // namespace xquery
