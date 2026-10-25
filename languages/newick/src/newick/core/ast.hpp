// The newick language: its AST. A tree is a recursive Clade: an optional
// label, an optional branch length, and its children.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace newick::ast {

struct Clade : cursiv::Extends<cursiv::AstNode> {
  boolean hasName;
  string_view name;
  optional<string_view> length;
  vector<Clade*> children;
};

}  // namespace newick::ast
