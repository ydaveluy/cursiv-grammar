// The pii language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pii::ast {

// The issn- or isbn-based identifier a Doc carries; Issn/Isbn narrow it.
struct Identifier : cursiv::Extends<cursiv::AstNode> {};

struct Issn : cursiv::Extends<Identifier> {
  string_view digits;  // 7 digits and an ISSN check character (digit or X)
  string_view year;
};

struct Isbn : cursiv::Extends<Identifier> {
  string_view digits;  // compact ISBN-10 (possibly ending in X) or ISBN-13
};

struct Doc : cursiv::Extends<cursiv::AstNode> {
  Identifier* id;
  string_view item;   // 5 characters; an issue identifier may begin with X
  string_view check;  // one digit, or 'X'
};

}  // namespace pii::ast
