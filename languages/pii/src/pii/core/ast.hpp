// The pii language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pii::ast {

// The issn- or isbn-based identifier a Doc carries; Issn/Isbn narrow it.
struct Identifier : cursiv::Extends<cursiv::AstNode> {};

struct Issn : cursiv::Extends<Identifier> {
  string_view digits;  // the 8 compacted ISSN digits
  string_view year;
};

struct Isbn : cursiv::Extends<Identifier> {
  string_view digits;  // the 10 ISBN digits
};

struct Doc : cursiv::Extends<cursiv::AstNode> {
  Identifier* id;
  string_view item;   // 5 digits
  string_view check;  // one digit, or 'X'
};

}  // namespace pii::ast
