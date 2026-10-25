// The telephone language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace telephone::ast {

// The three digit groups every variation shares.
struct Variation : cursiv::Extends<cursiv::AstNode> {
  string_view areaCode;
  string_view exchange;
  string_view subscriber;
};

struct Nanp : cursiv::Extends<Variation> {};
struct Japan : cursiv::Extends<Variation> {};

struct Number : cursiv::Extends<cursiv::AstNode> {
  boolean hasCountryCode;  // '+1'?
  boolean hasPlus;         // '+'?
  Variation* variation;
};

}  // namespace telephone::ast
