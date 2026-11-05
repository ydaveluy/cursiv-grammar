// The newick language: its AST. A tree is a recursive Clade: an optional
// label, an optional branch length, and its children.
#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace newick::ast {

struct Clade : cursiv::Extends<cursiv::AstNode> {
  boolean hasName;
  string name;
  optional<string> length;
  vector<Clade*> children;
};

}  // namespace newick::ast
