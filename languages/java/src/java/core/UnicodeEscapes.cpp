#include "java/core/UnicodeEscapes.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include "cursiv/core/text/scalar_at.hpp"

namespace java {
namespace {

// Character.digit(char, 16), used by javac's UnicodeReader, also accepts
// decimal digits outside ASCII and fullwidth A-F. These are the BMP decimal
// blocks (supplementary characters are two UTF-16 units, neither a digit).
int hex_digit(char32_t c) {
  if (c >= '0' && c <= '9') return int(c - '0');
  if (c >= 'a' && c <= 'f') return int(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return int(c - 'A' + 10);
  if (c >= 0xff41 && c <= 0xff46) return int(c - 0xff41 + 10);
  if (c >= 0xff21 && c <= 0xff26) return int(c - 0xff21 + 10);
  constexpr std::uint16_t zeroes[] = {
      0x660,  0x6f0,  0x7c0,  0x966,  0x9e6,  0xa66,  0xae6,  0xb66,
      0xbe6,  0xc66,  0xce6,  0xd66,  0xde6,  0xe50,  0xed0,  0xf20,
      0x1040, 0x1090, 0x17e0, 0x1810, 0x1946, 0x19d0, 0x1a80, 0x1a90,
      0x1b50, 0x1bb0, 0x1c40, 0x1c50, 0xa620, 0xa8d0, 0xa900, 0xa9d0,
      0xa9f0, 0xaa50, 0xabf0, 0xff10};
  for (const auto zero : zeroes)
    if (c >= zero && c < char32_t(zero) + 10) return int(c - zero);
  return -1;
}

bool high(std::uint32_t u) { return u >= 0xd800 && u <= 0xdbff; }
bool low(std::uint32_t u) { return u >= 0xdc00 && u <= 0xdfff; }

// One UTF-16 unit at a time, as UnicodeReader reads them. Only a backslash
// matters: any other unit just clears the backslash state, so a unit read
// outside an escape is one byte, whatever character it starts.
struct Reader {
  struct Unit {
    std::uint32_t value;
    std::size_t begin, end;
    bool escape;
  };
  static constexpr std::uint32_t kEnd = 0xffffffff;

  std::string_view source;
  cursiv::SourceTranslation& out;
  std::size_t pos = 0;
  bool wasBackslash = false;
  bool wasUnicodeEscape = false;
  // A lookahead that finds no low surrogate is read again: its malformed
  // escapes are reported once, and dropped from the text on the second read.
  std::size_t reported = std::string_view::npos;

  // UnicodeReader.nextCodeUnit, including the parity after an escape that
  // produces a backslash and the skip over a malformed escape.
  Unit next(bool commit) {
    for (;;) {
      if (pos == source.size()) return {kEnd, pos, pos, false};
      const std::size_t begin = pos;
      if (source[pos] != '\\' || (wasBackslash && !wasUnicodeEscape)) {
        wasBackslash = wasUnicodeEscape = false;
        ++pos;
        return {std::uint32_t(static_cast<unsigned char>(source[begin])),
                begin, pos, false};
      }
      std::size_t p = pos + 1;
      while (p < source.size() && source[p] == 'u') ++p;
      if (p == pos + 1) {
        wasUnicodeEscape = false;
        wasBackslash = !wasBackslash;
        pos = p;
        return {'\\', begin, p, false};
      }
      std::uint32_t value = 0;
      unsigned digits = 0;
      for (; digits < 4 && p < source.size(); ++digits) {
        std::size_t length = 0;
        const int d = hex_digit(
            cursiv::scalar_at(source.data() + p, source.size() - p, length));
        if (d < 0) break;
        value = (value << 4) | unsigned(d);
        p += length;
      }
      pos = p;
      if (digits == 4) {
        wasUnicodeEscape = true;
        wasBackslash = value == '\\' && !wasBackslash;
        return {value, begin, p, true};
      }
      if (reported == std::string_view::npos || begin > reported) {
        out.error(begin, p, "Malformed Java Unicode escape");
        reported = begin;
      }
      if (commit) out.replace(begin, p, "");
    }
  }
};

}  // namespace

void translate_unicode_escapes(std::string_view source,
                               cursiv::SourceTranslation& out) {
  if (source.find("\\u") == std::string_view::npos) return;
  Reader reader{source, out};
  for (;;) {
    // Text before the next backslash is kept, and clears the state.
    const std::size_t backslash = source.find('\\', reader.pos);
    if (backslash == std::string_view::npos) return;
    if (backslash != reader.pos) {
      reader.wasBackslash = reader.wasUnicodeEscape = false;
      reader.pos = backslash;
    }
    auto unit = reader.next(true);
    if (unit.value == Reader::kEnd) return;
    if (high(unit.value)) {
      // javac restores the position, but keeps the lookahead's backslash
      // state, when no low surrogate follows.
      const std::size_t saved = reader.pos;
      const auto next = reader.next(false);
      if (low(next.value)) {
        unit.value = 0x10000 + ((unit.value - 0xd800) << 10) +
                     (next.value - 0xdc00);
        unit.end = next.end;
      } else {
        reader.pos = saved;
      }
    }
    if (!unit.escape) continue;
    std::string spelling;
    if (unit.value >= 0xd800 && unit.value <= 0xdfff) {
      // Cursiv's lexer reads scalars, not WTF-8: a lone UTF-16 unit is kept
      // as a literal escape.
      constexpr char digits[] = "0123456789ABCDEF";
      spelling = "\\u";
      for (int shift = 12; shift >= 0; shift -= 4)
        spelling += digits[(unit.value >> shift) & 15];
    } else {
      cursiv::append_scalar(spelling, unit.value);
    }
    out.replace(unit.begin, unit.end, spelling);
  }
}

}  // namespace java
