#pragma once

#include <string_view>

#include "cursiv/core/syntax-tree/SourceTranslation.hpp"

namespace java {

// Java's Unicode-input translation (JLS 3.3), with javac's rules: the parser
// reads the translated text, while every offset stays the document's.
void translate_unicode_escapes(std::string_view source,
                               cursiv::SourceTranslation& out);

}  // namespace java
