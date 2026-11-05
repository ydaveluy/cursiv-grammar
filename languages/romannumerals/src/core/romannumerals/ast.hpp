#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace romannumerals::ast {

struct RomanNumeral : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

}  // namespace romannumerals::ast
