// The gml language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

// Every class derives `cursiv::AstNode`

namespace gml::ast {

struct Kv;

struct Number : cursiv::Extends<cursiv::AstNode> {
  string_view literal;
};

// The shape of both the document root and a bracketed `[ ... ]` value: a
// run of key/value pairs.
struct List : cursiv::Extends<cursiv::AstNode> {
  vector<Kv*> entries;
};

// Namespace scope, so the AstNode vocabulary (`string_view`, `vector`) is
// out of reach here and the std spelling is the only one.
using Val = std::variant<Number*, std::string_view, List*>;

struct Kv : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  Val value;
  boolean quoted = false;
};

}  // namespace gml::ast
