// The bcl language: its AST. A term is a leaf combinator or an application
// of two terms; a leaf carries no data, so it is a value, not a node.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bcl::ast {

enum class Combinator { K, S };

struct Apply;

// Namespace scope, so the AstNode vocabulary (`vector`, `reference`) is out
// of reach here and the std spelling is the only one.
using Term = std::variant<Combinator, Apply*>;

struct Apply : cursiv::Extends<cursiv::AstNode> {
  Term left;
  Term right;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Term> terms;
};

}  // namespace bcl::ast
