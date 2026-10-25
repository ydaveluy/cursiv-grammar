#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sgf::ast {

struct Property : cursiv::Extends<cursiv::AstNode> {
  string_view id;
  vector<string_view> values;
};

struct Node : cursiv::Extends<cursiv::AstNode> {
  vector<Property*> properties;
};

struct GameTree : cursiv::Extends<cursiv::AstNode> {
  vector<Node*> nodes;
  vector<GameTree*> children;
};

struct Collection : cursiv::Extends<cursiv::AstNode> {
  vector<GameTree*> trees;
};

}  // namespace sgf::ast
