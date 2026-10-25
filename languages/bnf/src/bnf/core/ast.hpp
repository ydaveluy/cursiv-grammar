// The bnf language: its AST. A document is a list of productions; each
// production's right-hand side is alternatives of element sequences.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bnf::ast {

// A quoted string, a bracketed or bare nonterminal name, or one other
// character/word read as literal text.
struct Element : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  boolean nonterminal;
};

struct Alternative : cursiv::Extends<cursiv::AstNode> {
  vector<Element*> elements;
};

// An empty alternative represents epsilon.
struct Production : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Alternative*> alternatives;
};

struct Grammar : cursiv::Extends<cursiv::AstNode> {
  vector<Production*> productions;
};

}  // namespace bnf::ast
