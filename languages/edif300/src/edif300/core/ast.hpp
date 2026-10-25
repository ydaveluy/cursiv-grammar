// The edif300 language: its AST. EDIF is nested parenthesized forms --
// `(keyword arg...)` -- so one shape covers all ~670 constructs the grammar
// names: Form::keyword (with its leading '(') says which one a node is, and
// Form::args holds its children in source order, scalars inline.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace edif300::ast {

// Namespace scope, so the AstNode vocabulary (`string_view`, `vector`) is
// out of reach here and the std spelling is the only one.
using Value = std::variant<cursiv::AstNode*, std::string_view, double, bool>;

struct Form : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;
  vector<Value> args;
};

}  // namespace edif300::ast
