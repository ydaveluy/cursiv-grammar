#pragma once

#include <memory>

#include "cursiv/core/parser/Parser.hpp"

namespace java {

// The generated parser reads Java tokens. The language service first performs
// Java's Unicode-input translation, while exposing CST spans in the original
// document. The returned parser owns its lexical delegate.
std::unique_ptr<cursiv::Parser>
unicode_parser(std::unique_ptr<const cursiv::Parser> delegate);

} // namespace java
