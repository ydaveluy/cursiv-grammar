#pragma once

#include <string_view>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "cursiv/core/syntax-tree/SourceTranslation.hpp"

namespace scala {
struct UnicodeSourceEscape {
  std::size_t end;
  std::uint16_t unit;
  bool valid;
};
// One eligible escape at this source offset, not a second preprocessing pass.
std::optional<UnicodeSourceEscape> unicode_escape_at(std::string_view source,
                                                   std::size_t begin);
void translate_unicode_source(std::string_view source,
                              cursiv::SourceTranslation& out);
}  // namespace scala
