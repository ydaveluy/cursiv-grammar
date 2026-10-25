#pragma once
#include "blueprint/core/ast.hpp"
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace blueprint {
inline std::optional<std::int64_t> integer_value(const ast::IntegerLit &n) {
  auto s = n.raw;
  if (s.starts_with('-'))
    s.remove_prefix(1);
  if (s.empty())
    return {};
  for (char c : s)
    if (c < '0' || c > '9')
      return {};
  if (s.size() > 1 && s.front() == '0' && s.find_first_of("89") != s.npos)
    return {};
  std::int64_t v;
  auto r = std::from_chars(n.raw.data(), n.raw.data() + n.raw.size(), v);
  if (r.ec != std::errc{} || r.ptr != n.raw.data() + n.raw.size())
    return {};
  return v;
}
struct StringValue {
  std::string text;
  std::string_view error;
};
namespace detail {
inline std::size_t scalar_width(std::string_view s) {
  if (s.empty())
    return 0;
  auto b = static_cast<unsigned char>(s.front());
  if (b < 0x80)
    return b ? 1 : 0;
  const auto n = b >= 0xc2 && b <= 0xdf   ? 2
                 : b >= 0xe0 && b <= 0xef ? 3
                 : b >= 0xf0 && b <= 0xf4 ? 4
                                          : 0;
  if (!n || s.size() < static_cast<std::size_t>(n))
    return 0;
  std::uint32_t cp = b & ((1u << (7 - n)) - 1u);
  for (int i = 1; i < n; ++i) {
    auto c = static_cast<unsigned char>(s[i]);
    if ((c & 0xc0) != 0x80)
      return 0;
    cp = (cp << 6) | (c & 0x3f);
  }
  if (cp < (n == 2   ? 0x80u
            : n == 3 ? 0x800u
                     : 0x10000u) ||
      cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
    return 0;
  return n;
}
inline void append_scalar(std::string &s, std::uint32_t cp) {
  if (cp < 0x80)
    s += static_cast<char>(cp);
  else if (cp < 0x800) {
    s += static_cast<char>(0xc0 | (cp >> 6));
    s += static_cast<char>(0x80 | (cp & 0x3f));
  } else if (cp < 0x10000) {
    s += static_cast<char>(0xe0 | (cp >> 12));
    s += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
    s += static_cast<char>(0x80 | (cp & 0x3f));
  } else {
    s += static_cast<char>(0xf0 | (cp >> 18));
    s += static_cast<char>(0x80 | ((cp >> 12) & 0x3f));
    s += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
    s += static_cast<char>(0x80 | (cp & 0x3f));
  }
}
inline int digit(char c) {
  return c >= '0' && c <= '9'   ? c - '0'
         : c >= 'a' && c <= 'f' ? c - 'a' + 10
         : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                : -1;
}
} // namespace detail
inline StringValue string_value(const ast::StringLit &n) {
  StringValue out;
  const auto raw = n.raw;
  if (raw.size() < 2 || raw.front() != raw.back() ||
      (raw.front() != '"' && raw.front() != '`')) {
    out.error = "A string requires matching quote delimiters.";
    return out;
  }
  const bool verbatim = raw.front() == '`';
  auto body = raw.substr(1, raw.size() - 2);
  for (std::size_t i = 0; i < body.size();) {
    char ch = body[i];
    if (!verbatim && ch == '\n') {
      out.error = "A quoted string cannot contain an unescaped line feed.";
      return out;
    }
    if (verbatim || ch != '\\') {
      const auto width = detail::scalar_width(body.substr(i));
      if (!width) {
        out.error = "String source contains NUL or invalid UTF-8.";
        return out;
      }
      if (!verbatim || ch != '\r')
        out.text.append(body.substr(i, width));
      i += width;
      continue;
    }
    if (++i == body.size()) {
      out.error = "The last backslash has no escaped character.";
      return out;
    }
    ch = body[i++];
    switch (ch) {
    case 'a':
      out.text += '\a';
      continue;
    case 'b':
      out.text += '\b';
      continue;
    case 'f':
      out.text += '\f';
      continue;
    case 'n':
      out.text += '\n';
      continue;
    case 'r':
      out.text += '\r';
      continue;
    case 't':
      out.text += '\t';
      continue;
    case 'v':
      out.text += '\v';
      continue;
    case '\\':
      out.text += '\\';
      continue;
    case '"':
      out.text += '"';
      continue;
    }
    unsigned count = ch == 'x'                ? 2
                     : ch == 'u'              ? 4
                     : ch == 'U'              ? 8
                     : ch >= '0' && ch <= '7' ? 3
                                              : 0;
    const bool octal = ch >= '0' && ch <= '7';
    if (!count) {
      out.error = "Unknown Go string escape.";
      return out;
    }
    std::uint32_t cp = 0;
    if (octal)
      --i;
    if (body.size() - i < count) {
      out.error = "The numeric escape is incomplete.";
      return out;
    }
    for (unsigned j = 0; j < count; ++j) {
      int d = detail::digit(body[i++]);
      if (d < 0 || d >= (octal ? 8 : 16)) {
        out.error = "The numeric escape contains an invalid digit.";
        return out;
      }
      cp = cp * (octal ? 8 : 16) + d;
    }
    if (octal || ch == 'x') {
      if (cp > 255) {
        out.error = "An octal byte escape must not exceed 377.";
        return out;
      }
      out.text += static_cast<char>(cp);
    } else {
      if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) {
        out.error = "The Unicode escape is not a Unicode scalar.";
        return out;
      }
      detail::append_scalar(out.text, cp);
    }
  }
  return out;
}
} // namespace blueprint
