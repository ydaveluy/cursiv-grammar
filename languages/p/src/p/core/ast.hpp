// The P'' language: its AST -- a tree of atoms (R, λ) and parenthesized
// groups, mirroring the grammar's own recursion.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace p::ast {

enum class AtomKind : std::uint8_t { r, l };

struct Symbol : cursiv::Extends<cursiv::AstNode> {};

struct Atom : cursiv::Extends<Symbol> {
  AtomKind kind;
};

struct Iterate : cursiv::Extends<Symbol> {
  vector<Symbol*> symbols;
};

struct Prog : cursiv::Extends<cursiv::AstNode> {
  vector<Symbol*> symbols;
};

}  // namespace p::ast
