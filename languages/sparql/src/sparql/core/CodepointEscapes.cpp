#include "sparql/core/CodepointEscapes.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include "cursiv/core/text/scalar_at.hpp"

namespace sparql {
namespace {

int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

}  // namespace

void translate_codepoint_escapes(std::string_view source,
                                 cursiv::SourceTranslation& out) {
  std::size_t pos = 0;
  while ((pos = source.find('\\', pos)) != std::string_view::npos) {
    const std::size_t begin = pos++;
    if (pos == source.size() || (source[pos] != 'u' && source[pos] != 'U'))
      continue;
    const std::size_t count = source[pos++] == 'u' ? 4 : 8;
    std::uint32_t cp = 0;
    std::size_t digits = 0;
    while (digits < count && digits < source.size() - pos) {
      const int digit = hex_digit(source[pos + digits]);
      if (digit < 0) break;
      cp = (cp << 4) | static_cast<unsigned>(digit);
      ++digits;
    }
    // Only the exact A.2 forms are substitutions. Other backslash text is
    // left to its lexical context (including ordinary comment contents).
    if (digits != count) continue;
    pos += count;
    if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) {
      out.error(begin, pos, "Invalid SPARQL Unicode scalar escape.");
      continue;
    }
    std::string spelling;
    cursiv::append_scalar(spelling, cp);
    out.replace(begin, pos, spelling);
  }
}

}  // namespace sparql
