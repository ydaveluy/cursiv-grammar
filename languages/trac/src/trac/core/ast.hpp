// The trac language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace trac::ast {

// The common base of Active and Neutral, so an Arg's value can hold either.
struct Macro : cursiv::Extends<cursiv::AstNode> {};

struct Arg : cursiv::Extends<cursiv::AstNode> {
  variant<Macro*, string_view> value;
};

struct Active : cursiv::Extends<Macro> {
  string_view name;
  vector<Arg*> args;
};

struct Neutral : cursiv::Extends<Macro> {
  string_view name;
  vector<Arg*> args;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Macro*> macros;
};

}  // namespace trac::ast
