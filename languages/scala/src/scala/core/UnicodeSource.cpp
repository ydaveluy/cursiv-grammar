#include "scala/core/UnicodeSource.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include "cursiv/core/text/scalar_at.hpp"

namespace scala {
namespace {
int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
}  // namespace

std::optional<UnicodeSourceEscape> unicode_escape_at(std::string_view source,
                                                   std::size_t begin) {
  if (begin >= source.size() || source[begin] != '\\' ||
      source.size() - begin < 2 || source[begin + 1] != 'u') return {};
  auto prefix = begin;
  while (prefix > 0 && source[prefix - 1] == '\\') --prefix;
  if ((begin - prefix) % 2 != 0) return {};
  auto pos = begin + 1;
  while (pos < source.size() && source[pos] == 'u') ++pos;
  std::uint32_t value = 0;
  std::size_t count = 0;
  while (count < 4 && count < source.size() - pos) {
    const int digit = hex_digit(source[pos + count]);
    if (digit < 0) break;
    value = (value << 4) | static_cast<unsigned>(digit);
    ++count;
  }
  if (count != 4) {
    const auto end = pos + count < source.size() ? pos + count + 1 : source.size();
    return UnicodeSourceEscape{end, 0, false};
  }
  return UnicodeSourceEscape{pos + 4, static_cast<std::uint16_t>(value), true};
}

void translate_unicode_source(std::string_view source,
                              cursiv::SourceTranslation& out) {
  std::size_t pos = 0;
  while ((pos = source.find('\\', pos)) != source.npos) {
    const auto begin = pos++;
    const auto escape = unicode_escape_at(source, begin);
    if (!escape) continue;
    pos = escape->end;
    if (!escape->valid) {
      out.error(begin, pos, "Invalid Scala Unicode escape: expected four hexadecimal digits.");
      continue;
    }
    // Scala literals and quoted names can contain a single UTF16 surrogate
    // unit. Their AST retains escape spelling; do not invent a UTF8 scalar,
    // join two Char units or reject a native-valid literal.
    if (escape->unit >= 0xd800 && escape->unit <= 0xdfff) continue;
    std::string spelling;
    cursiv::append_scalar(spelling, escape->unit);
    out.replace(begin, pos, spelling);
  }
}
}  // namespace scala
