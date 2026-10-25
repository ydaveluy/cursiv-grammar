#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace romannumerals::ast {

struct RomanNumeral : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

}  // namespace romannumerals::ast
