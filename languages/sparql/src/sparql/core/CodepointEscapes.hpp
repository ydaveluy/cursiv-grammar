#pragma once

#include <string_view>

#include "cursiv/core/syntax-tree/SourceTranslation.hpp"

namespace sparql {

// SPARQL1.0 Annex A.2: one global pass before tokenization, not a string or
// identifier escape. The official parser adapter preserves document offsets.
void translate_codepoint_escapes(std::string_view source,
                                 cursiv::SourceTranslation& out);

}  // namespace sparql
