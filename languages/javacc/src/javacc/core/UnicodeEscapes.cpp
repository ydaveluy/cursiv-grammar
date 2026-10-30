#include "javacc/core/UnicodeEscapes.hpp"
#include <cstdint>
#include <string>
#include "cursiv/core/text/scalar_at.hpp"

namespace javacc {
namespace {
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1; // JavaCC7 differs from javac: only ASCII hexadecimal digits.
}
struct Reader {
  struct Unit {
    std::uint32_t value;
    std::size_t begin, end;
    bool escaped = false, invalid = false;
  };
  static constexpr std::uint32_t end = 0xffffffff;
  std::string_view source;
  std::size_t at = 0, backslashes = 0;
  Unit next() {
    if (at == source.size()) return {end, at, at};
    const auto begin = at;
    if (source[at] != '\\') {
      backslashes = 0;
      std::size_t width = 0;
      const auto value = cursiv::scalar_at(source.data() + at, source.size() - at, width);
      at += width ? width : 1;
      return {std::uint32_t(value), begin, at};
    }
    ++at;
    const bool eligible = (backslashes++ & 1) == 0;
    if (!eligible || at == source.size() || source[at] != 'u') return {'\\', begin, at};
    while (at < source.size() && source[at] == 'u') ++at;
    backslashes = 0; // A translated backslash does not join a following raw run.
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) {
      if (at == source.size()) return {0, begin, at, false, true};
      const int digit = hex(source[at++]);
      if (digit < 0) return {0, begin, at, false, true};
      value = (value << 4) | unsigned(digit);
    }
    return {value, begin, at, true};
  }
};
} // namespace

void translate_unicode_escapes(std::string_view source,
                               cursiv::SourceTranslation& out) {
  if (source.find("\\u") == source.npos) return;
  Reader reader{source};
  for (;;) {
    auto unit = reader.next();
    if (unit.value == Reader::end) return;
    if (unit.invalid) {
      out.error(unit.begin, unit.end, "Malformed JavaCC Unicode escape");
      return; // The native JavaCharStream aborts on its first malformed escape.
    }
    if (!unit.escaped) continue;
    if (unit.value >= 0xd800 && unit.value <= 0xdbff) {
      const auto saved = reader;
      const auto low = reader.next();
      if (!low.invalid && low.value >= 0xdc00 && low.value <= 0xdfff) {
        unit.value = 0x10000 + ((unit.value - 0xd800) << 10) + low.value - 0xdc00;
        unit.end = low.end;
      } else reader = saved;
    }
    std::string spelling;
    if (unit.value >= 0xd800 && unit.value <= 0xdfff) {
      // Preserve a lone UTF-16 unit as an escape: Cursiv tokens read scalars.
      constexpr char digits[] = "0123456789ABCDEF";
      spelling = "\\u";
      for (int shift = 12; shift >= 0; shift -= 4) spelling += digits[(unit.value >> shift) & 15];
    } else cursiv::append_scalar(spelling, unit.value);
    out.replace(unit.begin, unit.end, spelling);
  }
}
} // namespace javacc
