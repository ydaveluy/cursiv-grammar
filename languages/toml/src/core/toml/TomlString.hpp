#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace toml {

struct StringResult {
  std::string text;
  std::string error;
  std::size_t error_offset = 0;
};

// Shared by AST conversion and runtime validation: invalid spellings still
// have text, while the validator can diagnose their original source position.
inline StringResult decode_string(std::string_view raw) {
  StringResult out;
  if (raw.size() < 2)
    return out;
  const char quote = raw[0];
  const bool basic = quote == '"';
  const bool multiline = raw.size() >= 6 && raw[1] == quote && raw[2] == quote;
  const std::size_t delimiter = multiline ? 3 : 1;
  std::size_t i = delimiter;
  const std::size_t end = raw.size() - delimiter;
  const auto error = [&](std::size_t at, std::string_view message) {
    if (out.error.empty()) {
      out.error = message;
      out.error_offset = at;
    }
  };
  const auto newline = [&](std::size_t at) -> std::size_t {
    if (at < end && raw[at] == '\n')
      return 1;
    if (at + 1 < end && raw[at] == '\r' && raw[at + 1] == '\n')
      return 2;
    return 0;
  };
  if (multiline)
    i += newline(i);
  while (i < end) {
    const auto c = static_cast<unsigned char>(raw[i]);
    if (basic && c == '\\') {
      const std::size_t start = i++;
      if (multiline) {
        auto next = i;
        while (next < end && (raw[next] == ' ' || raw[next] == '\t'))
          ++next;
        if (const auto n = newline(next)) {
          i = next + n;
          while (i < end) {
            if (raw[i] == ' ' || raw[i] == '\t')
              ++i;
            else if (const auto n2 = newline(i))
              i += n2;
            else
              break;
          }
          continue;
        }
      }
      if (i == end) {
        error(start, "Incomplete string escape.");
        out.text += '\\';
        break;
      }
      const char escaped = raw[i++];
      switch (escaped) {
      case 'b':
        out.text += '\b';
        break;
      case 't':
        out.text += '\t';
        break;
      case 'n':
        out.text += '\n';
        break;
      case 'f':
        out.text += '\f';
        break;
      case 'r':
        out.text += '\r';
        break;
      case '"':
        out.text += '"';
        break;
      case '\\':
        out.text += '\\';
        break;
      case 'u':
      case 'U': {
        const std::size_t count = escaped == 'u' ? 4 : 8;
        std::uint32_t scalar = 0;
        bool valid = i + count <= end;
        for (std::size_t j = 0; valid && j < count; ++j) {
          const char d = raw[i + j];
          unsigned digit = 0;
          if (d >= '0' && d <= '9')
            digit = d - '0';
          else if (d >= 'a' && d <= 'f')
            digit = d - 'a' + 10;
          else if (d >= 'A' && d <= 'F')
            digit = d - 'A' + 10;
          else {
            valid = false;
            break;
          }
          scalar = scalar * 16 + digit;
        }
        if (!valid) {
          error(start,
                "A Unicode escape needs four or eight hexadecimal digits.");
          out.text += raw.substr(start, 2);
          break;
        }
        i += count;
        if (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff)) {
          error(start, "The escape is not a Unicode scalar value.");
          out.text += raw.substr(start, count + 2);
        } else if (scalar < 0x80)
          out.text += static_cast<char>(scalar);
        else if (scalar < 0x800) {
          out.text += static_cast<char>(0xc0 | (scalar >> 6));
          out.text += static_cast<char>(0x80 | (scalar & 63));
        } else if (scalar < 0x10000) {
          out.text += static_cast<char>(0xe0 | (scalar >> 12));
          out.text += static_cast<char>(0x80 | ((scalar >> 6) & 63));
          out.text += static_cast<char>(0x80 | (scalar & 63));
        } else {
          out.text += static_cast<char>(0xf0 | (scalar >> 18));
          out.text += static_cast<char>(0x80 | ((scalar >> 12) & 63));
          out.text += static_cast<char>(0x80 | ((scalar >> 6) & 63));
          out.text += static_cast<char>(0x80 | (scalar & 63));
        }
        break;
      }
      default:
        error(start, "Unknown string escape.");
        out.text += '\\';
        out.text += escaped;
      }
      continue;
    }
    if (const auto n = newline(i)) {
      if (!multiline)
        error(i, "A single-line string cannot contain a newline.");
      out.text += '\n';
      i += n;
      continue;
    }
    if ((c < 0x20 && c != '\t') || c == 0x7f)
      error(i, "Unescaped control character in string.");
    out.text += static_cast<char>(c);
    ++i;
  }
  return out;
}

} // namespace toml
