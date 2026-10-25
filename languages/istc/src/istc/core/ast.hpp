// The istc language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace istc::ast {

struct Istc : cursiv::Extends<cursiv::AstNode> {
  string_view registration;
  string_view year;
  string_view work;
  string_view check;
};

}  // namespace istc::ast
