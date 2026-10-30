#pragma once
#include <string_view>
#include "cursiv/core/syntax-tree/SourceTranslation.hpp"

namespace javacc {
// JavaCC7 JavaCharStream, before tokenization; source remapping is Cursiv's.
void translate_unicode_escapes(std::string_view source,
                               cursiv::SourceTranslation& out);
} // namespace javacc
